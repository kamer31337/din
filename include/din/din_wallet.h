#ifndef DIN_WALLET_H
#define DIN_WALLET_H

#include "din_types.h"

#define DIN_MAX_WALLETS 64
#define DIN_MNEMONIC_WORDS 12
#define DIN_MNEMONIC_MAX_LEN 160
#define DIN_WALLET_LABEL_MAX 32
#define DIN_DID_MAX_LEN 64

typedef struct
{
    char label[DIN_WALLET_LABEL_MAX];
    din_role_t role;
    uint8_t private_key[DIN_HASH_LEN];
    uint8_t public_key[DIN_HASH_LEN * 2];
    din_addr_t address;
    char mnemonic[DIN_MNEMONIC_MAX_LEN];
    char did[DIN_DID_MAX_LEN];
    float usdc_balance;
    float eth_balance;
    uint32_t staked_nft_id;
    bool is_active;
} din_wallet_t;

typedef struct
{
    uint32_t count;
    uint32_t active_index;
    din_wallet_t wallets[DIN_MAX_WALLETS];
} din_wallet_manager_t;

din_error_t din_wallet_init_manager(din_wallet_manager_t *mgr);
din_error_t din_wallet_create(din_wallet_manager_t *mgr, const char *label, din_role_t role, din_wallet_t *out_wallet);
din_error_t din_wallet_import_mnemonic(din_wallet_manager_t *mgr, const char *mnemonic, const char *label, din_role_t role, din_wallet_t *out_wallet);
din_error_t din_wallet_import_private_key(din_wallet_manager_t *mgr, const uint8_t *priv_key, const char *label, din_role_t role, din_wallet_t *out_wallet);
din_error_t din_wallet_sign_data(const din_wallet_t *wallet, const uint8_t *data, size_t len, din_hash256_t *out_sig);
bool din_wallet_verify_sig(const din_wallet_t *wallet, const uint8_t *data, size_t len, const din_hash256_t *sig);
din_error_t din_wallet_fund(din_wallet_manager_t *mgr, const char *addr, float usdc_amount, float eth_amount);
din_error_t din_wallet_transfer_usdc(din_wallet_manager_t *mgr, const char *from_addr, const char *to_addr, float amount);
din_wallet_t *din_wallet_find_by_address(din_wallet_manager_t *mgr, const char *addr);
din_error_t din_wallet_save_file(const din_wallet_manager_t *mgr, const char *filepath);
din_error_t din_wallet_load_file(din_wallet_manager_t *mgr, const char *filepath);

#endif
