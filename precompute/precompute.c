#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/sha.h>

#define B 64
#define B_8 72
#define B_20 84
#define SECRET_LENGTH 20
#define SHA_DIGEST_LENGTH_m1 19
#define RAND_SIZE 0x1000000
#define RAND_SIZE_m_SECRET_LENGTH RAND_SIZE - SECRET_LENGTH

#define STEP_START {{STEP_START}}
#define STEP_A_DAY 2880
#define STEP_END STEP_START + STEP_A_DAY
#define ALL_SECRET_LENGTH STEP_A_DAY * SECRET_LENGTH
#define TARGET 000000

#define XOR_FAST(key, dst) \
    do { \
        ((uint64_t*)(dst))[0] ^= ((uint64_t*)(key))[0]; \
        ((uint64_t*)(dst))[1] ^= ((uint64_t*)(key))[1]; \
        ((uint32_t*)(dst))[4] ^= ((uint32_t*)(key))[4]; \
    } while(0)

void hmac(unsigned char *key, unsigned char *msg, unsigned char *mac) {
}

int main() {
    FILE *fp;
    unsigned char *rand;

    rand = (unsigned char *)malloc(RAND_SIZE);
    fp = fopen("/dev/urandom", "rb");
    fread(rand, 1, RAND_SIZE, fp);
    fclose(fp);

    int i, j;
    unsigned long step;
    unsigned char *secret;
    unsigned char cnt[8];
    unsigned char mac[SHA_DIGEST_LENGTH];
    size_t offset;
    int res;
    unsigned char all_secret[ALL_SECRET_LENGTH];
    SHA_CTX ctx;

    for (step = STEP_START; step < STEP_END; step++) {
        cnt[0] = (step >> 56) & 0xff;
        cnt[1] = (step >> 48) & 0xff;
        cnt[2] = (step >> 40) & 0xff;
        cnt[3] = (step >> 32) & 0xff;
        cnt[4] = (step >> 24) & 0xff;
        cnt[5] = (step >> 16) & 0xff;
        cnt[6] = (step >> 8) & 0xff;
        cnt[7] = step & 0xff;

        for (i = 0; i < RAND_SIZE_m_SECRET_LENGTH; i++) {
            secret = &rand[i];

            unsigned char k_ipad_msg[B_8] = {0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36};
            unsigned char k_opad_digest[B_20] = {0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c};

            XOR_FAST(secret, k_ipad_msg);
            for (j = 0; j < 8; j++) k_ipad_msg[B + j] = cnt[j];

            XOR_FAST(secret, k_opad_digest);
            
            SHA1_Init(&ctx);
            SHA1_Update(&ctx, k_ipad_msg, B_8);
            SHA1_Final(&k_opad_digest[B], &ctx);

            SHA1_Init(&ctx);
            SHA1_Update(&ctx, k_opad_digest, B_20);
            SHA1_Final(mac, &ctx);

            hmac(secret, cnt, mac);

            offset = mac[SHA_DIGEST_LENGTH_m1] & 0xf;
            res = (mac[offset] & 0x7f) * 0x1000000 + mac[offset + 1] * 0x10000 + mac[offset + 2] * 0x100 + mac[offset + 3];
            res %= 1000000;
            if (res == TARGET) {
                printf("\nFound (%ld)\nSECRET: ", step);
                
                for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
                    printf("%02x", secret[i]);
                }

                memcpy(&all_secret[(step - STEP_START) * SECRET_LENGTH], secret, 20);

                break;
            }
        }
    }

    fp = fopen("{{OUTPUT_FILE}}", "wb");
    fwrite(all_secret, 1, ALL_SECRET_LENGTH, fp);
    fclose(fp);

    return 0;
}