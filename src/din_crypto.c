#include "din/din_crypto.h"
#include <stdio.h>
#include <string.h>

#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

static const uint32_t k_sha256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

typedef struct
{
    uint8_t data[64];
    uint32_t datalen;
    unsigned long long bitlen;
    uint32_t state[8];
} sha256_ctx_t;

static void sha256_transform(sha256_ctx_t *ctx, const uint8_t *data)
{
    uint32_t a = ctx->state[0];
    uint32_t b = ctx->state[1];
    uint32_t c = ctx->state[2];
    uint32_t d = ctx->state[3];
    uint32_t e = ctx->state[4];
    uint32_t f = ctx->state[5];
    uint32_t g = ctx->state[6];
    uint32_t h = ctx->state[7];
    uint32_t m[64] = {0};

    for (uint32_t i = 0, j = 0; i < 16; ++i, j += 4)
    {
        m[i] = ((uint32_t)data[j] << 24) | ((uint32_t)data[j + 1] << 16) | ((uint32_t)data[j + 2] << 8) | ((uint32_t)data[j + 3]);
    }
    for (uint32_t i = 16; i < 64; ++i)
    {
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
    }
    for (uint32_t i = 0; i < 64; ++i)
    {
        uint32_t t1 = h + EP1(e) + CH(e, f, g) + k_sha256[i] + m[i];
        uint32_t t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

static void sha256_init(sha256_ctx_t *ctx)
{
    ctx->datalen = 0;
    ctx->bitlen = 0;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
}

static void sha256_update(sha256_ctx_t *ctx, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;
        if (ctx->datalen == 64)
        {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

static void sha256_final(sha256_ctx_t *ctx, uint8_t *hash)
{
    uint32_t i = ctx->datalen;

    if (ctx->datalen < 56)
    {
        ctx->data[i++] = 0x80;
        while (i < 56)
        {
            ctx->data[i++] = 0x00;
        }
    }
    else
    {
        ctx->data[i++] = 0x80;
        while (i < 64)
        {
            ctx->data[i++] = 0x00;
        }
        sha256_transform(ctx, ctx->data);
        memset(ctx->data, 0, 56);
    }
    ctx->bitlen += (unsigned long long)ctx->datalen * 8;
    ctx->data[63] = (uint8_t)(ctx->bitlen);
    ctx->data[62] = (uint8_t)(ctx->bitlen >> 8);
    ctx->data[61] = (uint8_t)(ctx->bitlen >> 16);
    ctx->data[60] = (uint8_t)(ctx->bitlen >> 24);
    ctx->data[59] = (uint8_t)(ctx->bitlen >> 32);
    ctx->data[58] = (uint8_t)(ctx->bitlen >> 40);
    ctx->data[57] = (uint8_t)(ctx->bitlen >> 48);
    ctx->data[56] = (uint8_t)(ctx->bitlen >> 56);
    sha256_transform(ctx, ctx->data);

    for (i = 0; i < 4; ++i)
    {
        hash[i] = (uint8_t)((ctx->state[0] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 4] = (uint8_t)((ctx->state[1] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 8] = (uint8_t)((ctx->state[2] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 12] = (uint8_t)((ctx->state[3] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 16] = (uint8_t)((ctx->state[4] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 20] = (uint8_t)((ctx->state[5] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 24] = (uint8_t)((ctx->state[6] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 28] = (uint8_t)((ctx->state[7] >> (24 - i * 8)) & 0x000000ff);
    }
}

void din_sha256(const uint8_t *data, size_t len, din_hash256_t *out_hash)
{
    sha256_ctx_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, out_hash->bytes);
}

void din_hash_to_hex(const din_hash256_t *hash, char *out_hex)
{
    static const char hex_chars[] = "0123456789abcdef";
    for (size_t i = 0; i < DIN_HASH_LEN; ++i)
    {
        out_hex[i * 2] = hex_chars[(hash->bytes[i] >> 4) & 0x0F];
        out_hex[i * 2 + 1] = hex_chars[hash->bytes[i] & 0x0F];
    }
    out_hex[DIN_HASH_LEN * 2] = '\0';
}

void din_create_cid(const uint8_t *data, size_t len, din_cid_t *out_cid)
{
    din_hash256_t hash;
    char hex[DIN_HASH_LEN * 2 + 1];
    din_sha256(data, len, &hash);
    din_hash_to_hex(&hash, hex);
    snprintf(out_cid->str, DIN_CID_MAX_LEN, "Qm%16.16s%32.32s", hex, hex + 16);
}

void din_commit_score(float score, uint32_t salt, din_hash256_t *out_commit)
{
    uint8_t buffer[sizeof(float) + sizeof(uint32_t)];
    memcpy(buffer, &score, sizeof(float));
    memcpy(buffer + sizeof(float), &salt, sizeof(uint32_t));
    din_sha256(buffer, sizeof(buffer), out_commit);
}

bool din_verify_score_reveal(float score, uint32_t salt, const din_hash256_t *commit)
{
    din_hash256_t check;
    din_commit_score(score, salt, &check);
    return memcmp(check.bytes, commit->bytes, DIN_HASH_LEN) == 0;
}

void din_generate_pairwise_mask(uint32_t party_a, uint32_t party_b, uint32_t round_num, float *mask_buffer, uint32_t size, float scale)
{
    if (party_a == party_b)
    {
        memset(mask_buffer, 0, sizeof(float) * size);
        return;
    }
    uint32_t min_id = (party_a < party_b) ? party_a : party_b;
    uint32_t max_id = (party_a < party_b) ? party_b : party_a;
    float sign = (party_a < party_b) ? 1.0f : -1.0f;
    uint32_t seed = (min_id * 10007) ^ (max_id * 38011) ^ (round_num * 53239);

    for (uint32_t i = 0; i < size; ++i)
    {
        seed = seed * 1664525u + 1013904223u;
        float normalized = ((float)(seed & 0xFFFF) / 65535.0f - 0.5f) * 2.0f;
        mask_buffer[i] = sign * normalized * scale;
    }
}
