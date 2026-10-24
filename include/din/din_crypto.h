#ifndef DIN_CRYPTO_H
#define DIN_CRYPTO_H

#include "din_types.h"

void din_sha256(const uint8_t *data, size_t len, din_hash256_t *out_hash);
void din_hash_to_hex(const din_hash256_t *hash, char *out_hex);
void din_create_cid(const uint8_t *data, size_t len, din_cid_t *out_cid);
void din_commit_score(float score, uint32_t salt, din_hash256_t *out_commit);
bool din_verify_score_reveal(float score, uint32_t salt, const din_hash256_t *commit);
void din_generate_pairwise_mask(uint32_t party_a, uint32_t party_b, uint32_t round_num, float *mask_buffer, uint32_t size, float scale);

#endif
