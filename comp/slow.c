#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

#define B 64
#define B_8 (B + 8)
#define B_20 (B + 20)
#define SECRET_LENGTH 20
#define SHA_DIGEST_LENGTH_m1 19
#define RAND_SIZE 0x1000000
#define RAND_SIZE_m_SECRET_LENGTH (RAND_SIZE - SECRET_LENGTH)

#define STEP_START 58923720
#define STEP_A_DAY 2880
#define STEP_END (STEP_START + STEP_A_DAY)
#define ALL_SECRET_LENGTH (STEP_A_DAY * SECRET_LENGTH)
#define TARGET 0

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

int totp(unsigned long step, unsigned char *secret) {
    unsigned char cnt[8];
    unsigned char mac[SHA_DIGEST_LENGTH];
    int res;
    size_t offset;

    cnt[0] = (step >> 56) & 0xff;
    cnt[1] = (step >> 48) & 0xff;
    cnt[2] = (step >> 40) & 0xff;
    cnt[3] = (step >> 32) & 0xff;
    cnt[4] = (step >> 24) & 0xff;
    cnt[5] = (step >> 16) & 0xff;
    cnt[6] = (step >> 8) & 0xff;
    cnt[7] = step & 0xff;

    hmac(secret, cnt, mac);
    offset = mac[SHA_DIGEST_LENGTH_m1] & 0xf;
    res = (mac[offset] & 0x7f) * 0x1000000 + mac[offset + 1] * 0x10000 + mac[offset + 2] * 0x100 + mac[offset + 3];
    res %= 1000000;

    return res;
}

int main() {
    unsigned long step;
    unsigned char secret[SECRET_LENGTH];
    unsigned char all_secret[ALL_SECRET_LENGTH];

    FILE *fp = fopen("/dev/urandom", "rb");

    for (step = STEP_START; step < STEP_END; step++) {
        printf("step %ld\n", step);
        for (int i = 0; i < RAND_SIZE; i++) {
            fread(secret, 1, SECRET_LENGTH, fp);
            if (totp(step, secret) == TARGET) {
                printf("Found\n");
                
                printf("SECRET: ");
                for (int i = 0; i < SECRET_LENGTH; i++) {
                    printf("%02x", secret[i]);
                }
                puts("");

                memcpy(&all_secret[(step - STEP_START) * SECRET_LENGTH], secret, 20);

                break;
            }
        }
    }
    
    fclose(fp);
    
    fp = fopen("output.bin", "wb");
    fwrite(all_secret, 1, ALL_SECRET_LENGTH, fp);
    fclose(fp);

    return 0;
}