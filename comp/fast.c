#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include <stdbool.h>

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

#define XOR_IPAD(key, dst) \
    do { \
        ((uint64_t*)(dst))[0] = ((uint64_t*)(key))[0] ^ 0x3636363636363636ULL; \
        ((uint64_t*)(dst))[1] = ((uint64_t*)(key))[1] ^ 0x3636363636363636ULL; \
        ((uint32_t*)(dst))[4] = ((uint32_t*)(key))[4] ^ 0x36363636U; \
    } while(0)

#define XOR_OPAD(key, dst) \
    do { \
        ((uint64_t*)(dst))[0] = ((uint64_t*)(key))[0] ^ 0x5c5c5c5c5c5c5c5cULL; \
        ((uint64_t*)(dst))[1] = ((uint64_t*)(key))[1] ^ 0x5c5c5c5c5c5c5c5cULL; \
        ((uint32_t*)(dst))[4] = ((uint32_t*)(key))[4] ^ 0x5c5c5c5cU; \
    } while(0)

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
    unsigned char mac[SHA_DIGEST_LENGTH];
    size_t offset;
    int res;
    unsigned char all_secret[ALL_SECRET_LENGTH];
    SHA_CTX ctx;
    SHA_CTX ctx_ipad_base;
    SHA_CTX ctx_opad_base;

    static const unsigned char ipad_rest[44] = {
        0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
        0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
        0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
        0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
        0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
        0x36, 0x36, 0x36, 0x36
    };
    static const unsigned char opad_rest[44] = {
        0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
        0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
        0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
        0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
        0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
        0x5c, 0x5c, 0x5c, 0x5c
    };

    bool found_map[STEP_A_DAY] = {0};
    int found_count = 0;

    unsigned char cnt_table[8 * STEP_A_DAY];
    unsigned long step_dif;
    unsigned char k_ipad[B];
    unsigned char k_opad[B];
    
    for (step = STEP_START; step < STEP_END; step++) {
        step_dif = step - STEP_START;
        cnt_table[step_dif * 8 + 0] = (step >> 56) & 0xff;
        cnt_table[step_dif * 8 + 1] = (step >> 48) & 0xff;
        cnt_table[step_dif * 8 + 2] = (step >> 40) & 0xff;
        cnt_table[step_dif * 8 + 3] = (step >> 32) & 0xff;
        cnt_table[step_dif * 8 + 4] = (step >> 24) & 0xff;
        cnt_table[step_dif * 8 + 5] = (step >> 16) & 0xff;
        cnt_table[step_dif * 8 + 6] = (step >> 8) & 0xff;
        cnt_table[step_dif * 8 + 7] = step & 0xff;
    }

    for (i = 0; i < RAND_SIZE_m_SECRET_LENGTH; i++) {
        secret = &rand[i];

        XOR_IPAD(secret, k_ipad);
        memcpy(&k_ipad[20], ipad_rest, 44);
        
        XOR_OPAD(secret, k_opad);
        memcpy(&k_opad[20], opad_rest, 44);
        
        SHA1_Init(&ctx_ipad_base);
        SHA1_Update(&ctx_ipad_base, k_ipad, 64);
        
        SHA1_Init(&ctx_opad_base);
        SHA1_Update(&ctx_opad_base, k_opad, 64);

        for (step = STEP_START; step < STEP_END; step++) {
            step_dif = step - STEP_START;

            if (found_map[step_dif]) {
                continue;
            }

            unsigned char inner_hash[20];

            memcpy(&ctx, &ctx_ipad_base, sizeof(SHA_CTX));
            SHA1_Update(&ctx, &cnt_table[step_dif * 8], 8);
            SHA1_Final(inner_hash, &ctx);
            
            memcpy(&ctx, &ctx_opad_base, sizeof(SHA_CTX));
            SHA1_Update(&ctx, inner_hash, 20);
            SHA1_Final(mac, &ctx);

            offset = mac[SHA_DIGEST_LENGTH_m1] & 0xf;
            res = (mac[offset] & 0x7f) * 0x1000000 + mac[offset + 1] * 0x10000 + mac[offset + 2] * 0x100 + mac[offset + 3];
            res %= 1000000;
            if (res == TARGET) {
                found_count++;
                found_map[step_dif] = true;

                printf("\nFound: %ld\nSECRET: ", step);
                
                for (int i = 0; i < SECRET_LENGTH; i++) {
                    printf("%02x", secret[i]);
                }

                memcpy(&all_secret[step_dif * SECRET_LENGTH], secret, 20);

                if (found_count == STEP_A_DAY) {
                    break;
                }
            }
        }
        if (found_count == STEP_A_DAY) {
            break;
        }
    }

    fp = fopen("output.bin", "wb");
    fwrite(all_secret, 1, ALL_SECRET_LENGTH, fp);
    fclose(fp);

    return 0;
}