#ifndef DIN_IPFS_H
#define DIN_IPFS_H

#include "din_types.h"
#include "din_model.h"

#define DIN_IPFS_MAX_OBJECTS 256
#define DIN_IPFS_MAX_BLOB_SIZE 8192

typedef struct
{
    din_cid_t cid;
    uint8_t data[DIN_IPFS_MAX_BLOB_SIZE];
    size_t size;
    bool occupied;
} din_ipfs_entry_t;

typedef struct
{
    din_ipfs_entry_t entries[DIN_IPFS_MAX_OBJECTS];
    uint32_t count;
} din_ipfs_t;

din_error_t din_ipfs_init(din_ipfs_t *ipfs);
din_error_t din_ipfs_put(din_ipfs_t *ipfs, const uint8_t *data, size_t len, din_cid_t *out_cid);
din_error_t din_ipfs_get(const din_ipfs_t *ipfs, const din_cid_t *cid, uint8_t *out_data, size_t max_len, size_t *out_len);
din_error_t din_ipfs_put_model(din_ipfs_t *ipfs, const din_model_t *model, din_cid_t *out_cid);
din_error_t din_ipfs_get_model(const din_ipfs_t *ipfs, const din_cid_t *cid, din_model_t *out_model);
bool din_ipfs_has(const din_ipfs_t *ipfs, const din_cid_t *cid);

#endif
