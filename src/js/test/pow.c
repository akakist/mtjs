#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* --- Минимальная реализация SHA-256 (короткая версия) --- */
/* Источник: публичный домен, сокращено для примера */

typedef struct {
    uint32_t h[8];
    uint64_t len;
    uint8_t buf[64];
    size_t buf_len;
} sha256_ctx;

static const uint32_t k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static void sha256_init(sha256_ctx *c) {
    c->h[0]=0x6a09e667; c->h[1]=0xbb67ae85; c->h[2]=0x3c6ef372; c->h[3]=0xa54ff53a;
    c->h[4]=0x510e527f; c->h[5]=0x9b05688c; c->h[6]=0x1f83d9ab; c->h[7]=0x5be0cd19;
    c->len = 0;
    c->buf_len = 0;
}

static void sha256_transform(sha256_ctx *c, const uint8_t *p) {
    uint32_t w[64], a,b,d,e,f,g,h,t1,t2;
    int i;

    for(i=0;i<16;i++){
        w[i] = (p[4*i]<<24)|(p[4*i+1]<<16)|(p[4*i+2]<<8)|p[4*i+3];
    }
    for(i=16;i<64;i++){
        uint32_t s0 = (w[i-15]>>7 | w[i-15]<<25) ^ (w[i-15]>>18 | w[i-15]<<14) ^ (w[i-15]>>3);
        uint32_t s1 = (w[i-2]>>17 | w[i-2]<<15) ^ (w[i-2]>>19 | w[i-2]<<13) ^ (w[i-2]>>10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }

    a=c->h[0]; b=c->h[1]; d=c->h[2]; e=c->h[3];
    f=c->h[4]; g=c->h[5]; h=c->h[6]; t1=c->h[7];

    for(i=0;i<64;i++){
        uint32_t S1 = (f>>6 | f<<26) ^ (f>>11 | f<<21) ^ (f>>25 | f<<7);
        uint32_t ch = (f & g) ^ ((~f) & h);
        uint32_t temp1 = t1 + S1 + ch + k[i] + w[i];
        uint32_t S0 = (a>>2 | a<<30) ^ (a>>13 | a<<19) ^ (a>>22 | a<<10);
        uint32_t maj = (a & b) ^ (a & d) ^ (b & d);
        uint32_t temp2 = S0 + maj;

        t1 = h;
        h = g;
        g = f;
        f = e + temp1;
        e = d;
        d = b;
        b = a;
        a = temp1 + temp2;
    }

    c->h[0]+=a; c->h[1]+=b; c->h[2]+=d; c->h[3]+=e;
    c->h[4]+=f; c->h[5]+=g; c->h[6]+=h; c->h[7]+=t1;
}

static void sha256_update(sha256_ctx *c, const void *data, size_t len) {
    const uint8_t *p = data;
    c->len += len;

    while(len--) {
        c->buf[c->buf_len++] = *p++;
        if(c->buf_len == 64) {
            sha256_transform(c, c->buf);
            c->buf_len = 0;
        }
    }
}

static void sha256_final(sha256_ctx *c, uint8_t out[32]) {
    uint64_t bit_len = c->len * 8;
    size_t i;

    c->buf[c->buf_len++] = 0x80;
    while(c->buf_len != 56) {
        if(c->buf_len == 64) {
            sha256_transform(c, c->buf);
            c->buf_len = 0;
        }
        c->buf[c->buf_len++] = 0;
    }

    for(i=0;i<8;i++)
        c->buf[56+i] = (bit_len >> (56 - 8*i)) & 0xff;

    sha256_transform(c, c->buf);

    for(i=0;i<8;i++){
        out[4*i]   = (c->h[i]>>24)&0xff;
        out[4*i+1] = (c->h[i]>>16)&0xff;
        out[4*i+2] = (c->h[i]>>8)&0xff;
        out[4*i+3] = c->h[i]&0xff;
    }
}
#include <stdint.h>
#include <stddef.h>

/* hash — массив из 32 байт (SHA-256)
 * bits — сколько ведущих нулевых бит нужно (N)
 * возвращает 1, если условие выполняется, иначе 0
 */
int hash_has_leading_zero_bits(const uint8_t hash[32], unsigned bits) {
    unsigned full_bytes = bits / 8;
    unsigned rem_bits   = bits % 8;

    // Проверяем полные байты
    for (unsigned i = 0; i < full_bytes; i++) {
        if (hash[i] != 0)
            return 0;
    }

    // Проверяем оставшиеся биты (если есть)
    if (rem_bits > 0) {
        uint8_t mask = (uint8_t)(0xFF << (8 - rem_bits)); // например, rem_bits=3 → 0b11100000
        if (hash[full_bytes] & mask)
            return 0;
    }

    return 1;
}

/* --- PoW на 24 бита --- */

int main() {
    const char *nick = "sergey";
    const char *pubkey = "ABC123PUBKEY1";
    uint32_t nonce = 0;
    uint8_t hash[32];

    char buf[256];

    while(1) {
        sha256_ctx ctx;
        sha256_init(&ctx);

        int n = snprintf(buf, sizeof(buf), "register:%s:%s:%u", nick, pubkey, nonce);
        sha256_update(&ctx, buf, n);
        sha256_final(&ctx, hash);

        /* Проверка: первые 3 байта == 0 */
        if(hash_has_leading_zero_bits(hash, 23)) {
            printf("FOUND NONCE: %u\n", nonce);
            printf("HASH: %02x%02x%02x...\n", hash[0], hash[1], hash[2]);
            break;
        }

        nonce++;
    }

    return 0;
}
