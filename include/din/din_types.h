#ifndef DIN_TYPES_H
#define DIN_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define DIN_CID_MAX_LEN 64
#define DIN_ADDR_MAX_LEN 48
#define DIN_MAX_PARTICIPANTS 128
#define DIN_MAX_EVALUATORS 32
#define DIN_MAX_SUBGROUPS 16
#define DIN_MAX_PARAMS 256
#define DIN_MAX_DATASET_SIZE 512
#define DIN_HASH_LEN 32

typedef enum
{
    DIN_OK = 0,
    DIN_ERR_INVALID_PARAM = -1,
    DIN_ERR_OUT_OF_MEMORY = -2,
    DIN_ERR_NOT_FOUND = -3,
    DIN_ERR_UNAUTHORIZED = -4,
    DIN_ERR_INSUFFICIENT_FUNDS = -5,
    DIN_ERR_BYZANTINE_DETECTED = -6,
    DIN_ERR_INVALID_STATE = -7,
    DIN_ERR_CRYPTO_FAIL = -8
} din_error_t;

typedef enum
{
    DIN_ROLE_NONE = 0,
    DIN_ROLE_MODEL_OWNER = 1,
    DIN_ROLE_PARTICIPANT = 2,
    DIN_ROLE_EVALUATOR = 3,
    DIN_ROLE_AGGREGATOR = 4
} din_role_t;

typedef enum
{
    DIN_PHASE_UNINITIALIZED = 0,
    DIN_PHASE_GENESIS = 1,
    DIN_PHASE_TRAINING = 2,
    DIN_PHASE_AGGREGATION = 3,
    DIN_PHASE_AUDITING = 4,
    DIN_PHASE_REWARD_SETTLED = 5,
    DIN_PHASE_COMPLETED = 6
} din_phase_t;

typedef struct
{
    char str[DIN_CID_MAX_LEN];
} din_cid_t;

typedef struct
{
    char str[DIN_ADDR_MAX_LEN];
} din_addr_t;

typedef struct
{
    uint8_t bytes[DIN_HASH_LEN];
} din_hash256_t;

#endif
