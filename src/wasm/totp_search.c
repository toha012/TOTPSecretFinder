#include <stdint.h>
#include <stddef.h>

#define SHA1_DIGEST_LENGTH 20

#define WASM_EXPORT __attribute__((visibility("default")))

static void my_memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) *d++ = *s++;
}

typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    uint8_t buffer[64];
} SHA1_CTX;

#define ROL(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

#define BLK0(i) (block[i] = __builtin_bswap32(block[i]))
#define BLK(i) (block[i&15] = ROL(block[(i+13)&15]^block[(i+8)&15]^block[(i+2)&15]^block[i&15],1))

#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+BLK0(i)+0x5A827999+ROL(v,5);w=ROL(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+BLK(i)+0x5A827999+ROL(v,5);w=ROL(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+BLK(i)+0x6ED9EBA1+ROL(v,5);w=ROL(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+BLK(i)+0x8F1BBCDC+ROL(v,5);w=ROL(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+BLK(i)+0xCA62C1D6+ROL(v,5);w=ROL(w,30);

static void SHA1_Transform(uint32_t state[5], const uint8_t buffer[64]) {
    uint32_t a, b, c, d, e;
    uint32_t block[16];
    my_memcpy(block, buffer, 64);
    
    a = state[0]; b = state[1]; c = state[2]; d = state[3]; e = state[4];
    
    R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
    R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
    R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
    R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
    R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
    R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
    R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
    R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
    R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
    R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
    R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
    R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
    R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
    R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
    R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
    R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
    R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
    R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
    R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
    R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);
    
    state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
}

static void SHA1_Init(SHA1_CTX *ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count[0] = ctx->count[1] = 0;
}

static void SHA1_Update(SHA1_CTX *ctx, const uint8_t *data, size_t len) {
    size_t i, j;
    j = (ctx->count[0] >> 3) & 63;
    if ((ctx->count[0] += len << 3) < (len << 3)) ctx->count[1]++;
    ctx->count[1] += (len >> 29);
    if ((j + len) > 63) {
        my_memcpy(&ctx->buffer[j], data, (i = 64 - j));
        SHA1_Transform(ctx->state, ctx->buffer);
        for (; i + 63 < len; i += 64) {
            SHA1_Transform(ctx->state, &data[i]);
        }
        j = 0;
    } else i = 0;
    my_memcpy(&ctx->buffer[j], &data[i], len - i);
}

static void SHA1_Final(uint8_t digest[20], SHA1_CTX *ctx) {
    uint8_t finalcount[8];
    for (int i = 0; i < 8; i++) {
        finalcount[i] = (uint8_t)((ctx->count[(i >= 4 ? 0 : 1)] >> ((3 - (i & 3)) * 8)) & 255);
    }
    uint8_t c = 0200;
    SHA1_Update(ctx, &c, 1);
    while ((ctx->count[0] & 504) != 448) {
        c = 0;
        SHA1_Update(ctx, &c, 1);
    }
    SHA1_Update(ctx, finalcount, 8);
    for (int i = 0; i < 20; i++) {
        digest[i] = (uint8_t)((ctx->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
    }
}

#define B 64
#define B_8 72
#define B_20 84
#define SECRET_LENGTH 20

#define XOR_FAST(key, dst) \
    do { \
        ((uint64_t*)(dst))[0] ^= ((uint64_t*)(key))[0]; \
        ((uint64_t*)(dst))[1] ^= ((uint64_t*)(key))[1]; \
        ((uint32_t*)(dst))[4] ^= ((uint32_t*)(key))[4]; \
    } while(0)

static uint8_t *g_random_data = 0;
static uint32_t g_random_size = 0;
static volatile int g_cancelled = 0;

static uint8_t g_found_secret[SECRET_LENGTH];

extern unsigned char __heap_base;
static size_t heap_offset = 0;

static void* simple_malloc(size_t size) {
    uint8_t *ptr = &__heap_base + heap_offset;
    heap_offset += size;
    heap_offset = (heap_offset + 7) & ~7;
    return ptr;
}

WASM_EXPORT
uint8_t* get_random_buffer(void) {
    return g_random_data;
}

WASM_EXPORT
int init_random_data(uint32_t size) {
    g_random_data = (uint8_t*)simple_malloc(size);
    if (g_random_data == 0) {
        return -1;
    }
    g_random_size = size;
    g_cancelled = 0;
    return 0;
}

WASM_EXPORT
void cancel_search(void) {
    g_cancelled = 1;
}

WASM_EXPORT
int search_step(uint32_t step_low, uint32_t step_high, uint32_t target) {
    uint64_t step = ((uint64_t)step_high << 32) | step_low;
    
    if (g_random_data == 0 || g_random_size < SECRET_LENGTH) {
        return -1;
    }
    
    uint8_t cnt[8];
    cnt[0] = (step >> 56) & 0xff;
    cnt[1] = (step >> 48) & 0xff;
    cnt[2] = (step >> 40) & 0xff;
    cnt[3] = (step >> 32) & 0xff;
    cnt[4] = (step >> 24) & 0xff;
    cnt[5] = (step >> 16) & 0xff;
    cnt[6] = (step >> 8) & 0xff;
    cnt[7] = step & 0xff;
    
    uint32_t max_index = g_random_size - SECRET_LENGTH;
    SHA1_CTX ctx;
    uint8_t mac[SHA1_DIGEST_LENGTH];
    
    for (uint32_t i = 0; i < max_index; i++) {
        if (g_cancelled) {
            return -2;
        }
        
        uint8_t *secret = &g_random_data[i];
        
        uint8_t k_ipad_msg[B_8] = {
            0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
            0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
            0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
            0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
            0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
            0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
            0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
            0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        
        uint8_t k_opad_digest[B_20] = {
            0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
            0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
            0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
            0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
            0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
            0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
            0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
            0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00
        };
        
        XOR_FAST(secret, k_ipad_msg);
        for (int j = 0; j < 8; j++) {
            k_ipad_msg[B + j] = cnt[j];
        }
        
        XOR_FAST(secret, k_opad_digest);
        
        SHA1_Init(&ctx);
        SHA1_Update(&ctx, k_ipad_msg, B_8);
        SHA1_Final(&k_opad_digest[B], &ctx);
        
        SHA1_Init(&ctx);
        SHA1_Update(&ctx, k_opad_digest, B_20);
        SHA1_Final(mac, &ctx);
        
        size_t offset = mac[SHA1_DIGEST_LENGTH - 1] & 0xf;
        uint32_t res = (mac[offset] & 0x7f) * 0x1000000 + mac[offset + 1] * 0x10000 + mac[offset + 2] * 0x100 + mac[offset + 3];
        res %= 1000000;
        
        if (res == target) {
            for (int k = 0; k < SECRET_LENGTH; k++) {
                g_found_secret[k] = secret[k];
            }
            return (int)i;
        }
    }
    
    return -1;
}

WASM_EXPORT
uint8_t* get_found_secret(void) {
    return g_found_secret;
}

WASM_EXPORT
uint32_t get_random_size(void) {
    return g_random_size;
}

WASM_EXPORT
uint32_t get_secret_length(void) {
    return SECRET_LENGTH;
}