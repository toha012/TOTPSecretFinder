#include <stdio.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>

#define B 64
#define B_8 (B + 8)
#define B_20 (B + 20)
#define SECRET_LENGTH 20
#define SHA_DIGEST_LENGTH_m1 19

#define X 30
#define T0 0

void xor(unsigned char *a, unsigned char *res, int length) {
    for (int i = 0; i < length; i++) {
        res[i] ^= a[i];
    }
}

void hmac(unsigned char *key, unsigned char *msg, unsigned char *mac) {
    int i;
    unsigned char ipad[B];
    unsigned char opad[B];
    unsigned char kipad[B] = {0};
    unsigned char kopad[B] = {0};
    unsigned char k_ipad_msg[B_8];
    unsigned char k_opad_digest[B_20];
    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA_CTX ctx;
    
    memset(ipad, '\x36', B);
    memset(opad, '\x5c', B);

    // kipad = K ^ ipad
    memcpy(kipad, key, SECRET_LENGTH);
    xor(ipad, kipad, B);

    // kopad = K ^ opad
    memcpy(kopad, key, SECRET_LENGTH);
    xor(opad, kopad, B);

    // k_ipad_msg = (K ^ ipad) || msg
    memcpy(k_ipad_msg, kipad, B);
    memcpy(&k_ipad_msg[B], msg, 8);

    // digest = SHA1((K ^ ipad) || msg)
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, k_ipad_msg, B_8);
    SHA1_Final(digest, &ctx);

    // k_opad_digest = (K ^ opad) || SHA1((K ^ ipad) || msg)
    memcpy(k_opad_digest, kopad, B);
    memcpy(&k_opad_digest[B], digest, SHA_DIGEST_LENGTH);

    // mac = SHA1((K ^ opad) || SHA1((K ^ ipad) || msg))
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, k_opad_digest, B_20);
    SHA1_Final(mac, &ctx);
}

int hotp(unsigned char *secret, unsigned char *step) {
    unsigned char hs[SHA_DIGEST_LENGTH];
    int offset;
    unsigned int p;
    int i;

    hmac(secret, step, hs);
    offset = hs[SHA_DIGEST_LENGTH_m1] & 0xf;

    p = 0;
    for (i = 0; i < 4; i++) p += (unsigned int)hs[offset + i] << (24 - i * 8);

    return (p & 0x7fffffff) % 1000000;
}

int totp(unsigned char *secret) {
    time_t cut;
    unsigned long step_long;
    unsigned char step[8];
    int i;

    cut = time(NULL);
    step_long = (cut - T0) / X;
    for (i = 0; i < 8; i++) step[i] = (step_long >> (56 - i * 8)) & 0xff;

    return hotp(secret, step);
}