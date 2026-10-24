#include "din/din_ipfs.h"
#include "din/din_crypto.h"
#include <string.h>

din_error_t din_ipfs_init(din_ipfs_t *ipfs)
{
    if (!ipfs)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    ipfs->count = 0;
    for (uint32_t i = 0; i < DIN_IPFS_MAX_OBJECTS; ++i)
    {
        ipfs->entries[i].occupied = false;
        ipfs->entries[i].size = 0;
        ipfs->entries[i].cid.str[0] = '\0';
    }
    return DIN_OK;
}

din_error_t din_ipfs_put(din_ipfs_t *ipfs, const uint8_t *data, size_t len, din_cid_t *out_cid)
{
    if (!ipfs || !data || len == 0 || len > DIN_IPFS_MAX_BLOB_SIZE || !out_cid)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    din_cid_t cid;
    din_create_cid(data, len, &cid);
    for (uint32_t i = 0; i < DIN_IPFS_MAX_OBJECTS; ++i)
    {
        if (ipfs->entries[i].occupied && strcmp(ipfs->entries[i].cid.str, cid.str) == 0)
        {
            *out_cid = cid;
            return DIN_OK;
        }
    }
    for (uint32_t i = 0; i < DIN_IPFS_MAX_OBJECTS; ++i)
    {
        if (!ipfs->entries[i].occupied)
        {
            ipfs->entries[i].occupied = true;
            ipfs->entries[i].size = len;
            ipfs->entries[i].cid = cid;
            memcpy(ipfs->entries[i].data, data, len);
            ipfs->count++;
            *out_cid = cid;
            return DIN_OK;
        }
    }
    return DIN_ERR_OUT_OF_MEMORY;
}

din_error_t din_ipfs_get(const din_ipfs_t *ipfs, const din_cid_t *cid, uint8_t *out_data, size_t max_len, size_t *out_len)
{
    if (!ipfs || !cid || !out_data || !out_len)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    for (uint32_t i = 0; i < DIN_IPFS_MAX_OBJECTS; ++i)
    {
        if (ipfs->entries[i].occupied && strcmp(ipfs->entries[i].cid.str, cid->str) == 0)
        {
            if (ipfs->entries[i].size > max_len)
            {
                return DIN_ERR_OUT_OF_MEMORY;
            }
            memcpy(out_data, ipfs->entries[i].data, ipfs->entries[i].size);
            *out_len = ipfs->entries[i].size;
            return DIN_OK;
        }
    }
    return DIN_ERR_NOT_FOUND;
}

din_error_t din_ipfs_put_model(din_ipfs_t *ipfs, const din_model_t *model, din_cid_t *out_cid)
{
    if (!ipfs || !model || !out_cid)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    uint8_t buffer[DIN_IPFS_MAX_BLOB_SIZE];
    size_t written = din_model_serialize(model, buffer, DIN_IPFS_MAX_BLOB_SIZE);
    if (written == 0)
    {
        return DIN_ERR_OUT_OF_MEMORY;
    }
    return din_ipfs_put(ipfs, buffer, written, out_cid);
}

din_error_t din_ipfs_get_model(const din_ipfs_t *ipfs, const din_cid_t *cid, din_model_t *out_model)
{
    if (!ipfs || !cid || !out_model)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    uint8_t buffer[DIN_IPFS_MAX_BLOB_SIZE];
    size_t actual_len = 0;
    din_error_t err = din_ipfs_get(ipfs, cid, buffer, DIN_IPFS_MAX_BLOB_SIZE, &actual_len);
    if (err != DIN_OK)
    {
        return err;
    }
    return din_model_deserialize(out_model, buffer, actual_len);
}

bool din_ipfs_has(const din_ipfs_t *ipfs, const din_cid_t *cid)
{
    if (!ipfs || !cid)
    {
        return false;
    }
    for (uint32_t i = 0; i < DIN_IPFS_MAX_OBJECTS; ++i)
    {
        if (ipfs->entries[i].occupied && strcmp(ipfs->entries[i].cid.str, cid->str) == 0)
        {
            return true;
        }
    }
    return false;
}
