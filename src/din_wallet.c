#include "din/din_wallet.h"
#include "din/din_crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *k_wordlist[256] = {
    "alpha", "anchor", "anthem", "apex", "armor", "arrow", "atlas", "atom",
    "aurora", "beacon", "binary", "blade", "blaze", "blend", "bloom", "bridge",
    "bronze", "buffer", "cactus", "canvas", "canyon", "carbon", "castle", "cedar",
    "cipher", "circuit", "cliff", "cloud", "clover", "cobalt", "comet", "compass",
    "copper", "coral", "cosmos", "cradle", "crater", "creek", "crest", "crystal",
    "dagger", "delta", "desert", "device", "diamond", "drift", "dune", "eagle",
    "earth", "echo", "eclipse", "ember", "emerald", "engine", "entropy", "epoch",
    "falcon", "feather", "flare", "flight", "flint", "forest", "fossil", "frost",
    "galaxy", "garden", "garnet", "geyser", "glacier", "glade", "glow", "granite",
    "grove", "harbor", "haven", "hawk", "helium", "helix", "helmet", "horizon",
    "hunter", "hybrid", "icicle", "impact", "impulse", "indigo", "island", "jaguar",
    "jasper", "javelin", "jungle", "kernel", "kinetic", "lagoon", "lantern", "laser",
    "lattice", "laurel", "legend", "lemon", "leopard", "level", "light", "lightning",
    "linear", "lizard", "logic", "lunar", "magnet", "mantle", "maple", "marble",
    "matrix", "meadow", "meteor", "mimic", "mirror", "monolith", "moon", "mosaic",
    "motion", "mountain", "nebula", "nectar", "network", "nexus", "nickel", "nitrogen",
    "nomad", "nova", "oasis", "ocean", "olive", "onyx", "optics", "orbit",
    "orchid", "oxygen", "packet", "palace", "panther", "particle", "path", "pebble",
    "phantom", "phoenix", "photon", "pillar", "pixel", "planet", "plasma", "platinum",
    "plume", "polar", "prism", "proto", "pulsar", "pulse", "pyramid", "quantum",
    "quark", "quartz", "quasar", "radar", "radiant", "radius", "raven", "reactor",
    "refine", "relic", "resonance", "ridge", "rift", "river", "rocket", "ruby",
    "saffron", "sailor", "sapphire", "satellite", "scalar", "scepter", "scout", "scroll",
    "sector", "sensor", "serpent", "shadow", "shield", "sierra", "signal", "silver",
    "siren", "solar", "solis", "sonar", "spark", "spectrum", "sphere", "spiral",
    "spring", "spruce", "star", "static", "strata", "stream", "strike", "summit",
    "sunlight", "surge", "synapse", "synergy", "system", "talon", "temple", "tensor",
    "terra", "thermal", "thunder", "tide", "timber", "titan", "topaz", "torch",
    "torque", "trace", "transit", "trigger", "tundra", "turbine", "umbra", "valiant",
    "valley", "vapor", "vault", "vector", "velocity", "venus", "vertex", "vessel",
    "vortex", "vulcan", "walnut", "warp", "water", "wave", "whisper", "wild",
    "winter", "wisdom", "wolf", "zenith", "zephyr", "zero", "zinc", "zodiac"
};

static void derive_keys_and_address(din_wallet_t *wallet)
{
    din_hash256_t h1;
    din_hash256_t h2;
    din_sha256(wallet->private_key, DIN_HASH_LEN, &h1);
    memcpy(wallet->public_key, h1.bytes, DIN_HASH_LEN);
    din_sha256(h1.bytes, DIN_HASH_LEN, &h2);
    memcpy(wallet->public_key + DIN_HASH_LEN, h2.bytes, DIN_HASH_LEN);

    din_hash256_t addr_hash;
    din_sha256(wallet->public_key, sizeof(wallet->public_key), &addr_hash);

    char hex[DIN_HASH_LEN * 2 + 1];
    din_hash_to_hex(&addr_hash, hex);
    snprintf(wallet->address.str, DIN_ADDR_MAX_LEN, "0x%.40s", hex + 24);
    snprintf(wallet->did, DIN_DID_MAX_LEN, "did:din:%s", wallet->address.str);
}

din_error_t din_wallet_init_manager(din_wallet_manager_t *mgr)
{
    if (!mgr)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    mgr->count = 0;
    mgr->active_index = 0;
    for (uint32_t i = 0; i < DIN_MAX_WALLETS; ++i)
    {
        mgr->wallets[i].is_active = false;
        mgr->wallets[i].label[0] = '\0';
        mgr->wallets[i].address.str[0] = '\0';
        mgr->wallets[i].mnemonic[0] = '\0';
        mgr->wallets[i].did[0] = '\0';
        mgr->wallets[i].usdc_balance = 0.0f;
        mgr->wallets[i].eth_balance = 0.0f;
        mgr->wallets[i].staked_nft_id = 0;
    }
    return DIN_OK;
}

din_error_t din_wallet_create(din_wallet_manager_t *mgr, const char *label, din_role_t role, din_wallet_t *out_wallet)
{
    if (!mgr || !label || mgr->count >= DIN_MAX_WALLETS)
    {
        return DIN_ERR_OUT_OF_MEMORY;
    }
    uint32_t idx = mgr->count;
    din_wallet_t *w = &mgr->wallets[idx];
    memset(w, 0, sizeof(din_wallet_t));
    snprintf(w->label, DIN_WALLET_LABEL_MAX, "%s", label);
    w->role = role;
    w->is_active = true;
    w->usdc_balance = 0.0f;
    w->eth_balance = 1.0f;
    w->staked_nft_id = 0;

    static uint32_t entropy_counter = 10007;
    entropy_counter = entropy_counter * 1664525u + (uint32_t)time(NULL) + (idx * 31);
    uint8_t entropy[12];
    for (uint32_t i = 0; i < 12; ++i)
    {
        entropy_counter = entropy_counter * 1664525u + 1013904223u;
        entropy[i] = (uint8_t)(entropy_counter >> 16);
    }

    char mnemonic_buf[DIN_MNEMONIC_MAX_LEN] = {0};
    size_t offset = 0;
    for (uint32_t i = 0; i < DIN_MNEMONIC_WORDS; ++i)
    {
        uint8_t word_idx = entropy[i];
        const char *word = k_wordlist[word_idx];
        offset += snprintf(mnemonic_buf + offset, sizeof(mnemonic_buf) - offset, "%s%s", word, (i + 1 < DIN_MNEMONIC_WORDS) ? " " : "");
    }
    snprintf(w->mnemonic, DIN_MNEMONIC_MAX_LEN, "%s", mnemonic_buf);

    din_hash256_t seed_hash;
    din_sha256((const uint8_t *)w->mnemonic, strlen(w->mnemonic), &seed_hash);
    memcpy(w->private_key, seed_hash.bytes, DIN_HASH_LEN);
    derive_keys_and_address(w);

    mgr->count++;
    if (out_wallet)
    {
        *out_wallet = *w;
    }
    return DIN_OK;
}

din_error_t din_wallet_import_mnemonic(din_wallet_manager_t *mgr, const char *mnemonic, const char *label, din_role_t role, din_wallet_t *out_wallet)
{
    if (!mgr || !mnemonic || !label || mgr->count >= DIN_MAX_WALLETS)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    uint32_t idx = mgr->count;
    din_wallet_t *w = &mgr->wallets[idx];
    memset(w, 0, sizeof(din_wallet_t));
    snprintf(w->label, DIN_WALLET_LABEL_MAX, "%s", label);
    w->role = role;
    w->is_active = true;
    w->usdc_balance = 0.0f;
    w->eth_balance = 1.0f;
    w->staked_nft_id = 0;
    snprintf(w->mnemonic, DIN_MNEMONIC_MAX_LEN, "%s", mnemonic);

    din_hash256_t seed_hash;
    din_sha256((const uint8_t *)w->mnemonic, strlen(w->mnemonic), &seed_hash);
    memcpy(w->private_key, seed_hash.bytes, DIN_HASH_LEN);
    derive_keys_and_address(w);

    mgr->count++;
    if (out_wallet)
    {
        *out_wallet = *w;
    }
    return DIN_OK;
}

din_error_t din_wallet_import_private_key(din_wallet_manager_t *mgr, const uint8_t *priv_key, const char *label, din_role_t role, din_wallet_t *out_wallet)
{
    if (!mgr || !priv_key || !label || mgr->count >= DIN_MAX_WALLETS)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    uint32_t idx = mgr->count;
    din_wallet_t *w = &mgr->wallets[idx];
    memset(w, 0, sizeof(din_wallet_t));
    snprintf(w->label, DIN_WALLET_LABEL_MAX, "%s", label);
    w->role = role;
    w->is_active = true;
    w->usdc_balance = 0.0f;
    w->eth_balance = 1.0f;
    w->staked_nft_id = 0;
    memcpy(w->private_key, priv_key, DIN_HASH_LEN);
    snprintf(w->mnemonic, DIN_MNEMONIC_MAX_LEN, "[Imported from Raw Private Key]");
    derive_keys_and_address(w);

    mgr->count++;
    if (out_wallet)
    {
        *out_wallet = *w;
    }
    return DIN_OK;
}

din_error_t din_wallet_sign_data(const din_wallet_t *wallet, const uint8_t *data, size_t len, din_hash256_t *out_sig)
{
    if (!wallet || !data || len == 0 || !out_sig)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    uint8_t buffer[DIN_HASH_LEN + 1024];
    size_t payload_len = (len > 1024) ? 1024 : len;
    memcpy(buffer, wallet->private_key, DIN_HASH_LEN);
    memcpy(buffer + DIN_HASH_LEN, data, payload_len);
    din_sha256(buffer, DIN_HASH_LEN + payload_len, out_sig);
    return DIN_OK;
}

bool din_wallet_verify_sig(const din_wallet_t *wallet, const uint8_t *data, size_t len, const din_hash256_t *sig)
{
    if (!wallet || !data || len == 0 || !sig)
    {
        return false;
    }
    din_hash256_t expected_sig;
    din_wallet_sign_data(wallet, data, len, &expected_sig);
    return memcmp(expected_sig.bytes, sig->bytes, DIN_HASH_LEN) == 0;
}

din_error_t din_wallet_fund(din_wallet_manager_t *mgr, const char *addr, float usdc_amount, float eth_amount)
{
    if (!mgr || !addr)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    din_wallet_t *w = din_wallet_find_by_address(mgr, addr);
    if (!w)
    {
        return DIN_ERR_NOT_FOUND;
    }
    w->usdc_balance += usdc_amount;
    w->eth_balance += eth_amount;
    return DIN_OK;
}

din_error_t din_wallet_transfer_usdc(din_wallet_manager_t *mgr, const char *from_addr, const char *to_addr, float amount)
{
    if (!mgr || !from_addr || !to_addr || amount <= 0.0f)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    din_wallet_t *from = din_wallet_find_by_address(mgr, from_addr);
    din_wallet_t *to = din_wallet_find_by_address(mgr, to_addr);
    if (!from || !to)
    {
        return DIN_ERR_NOT_FOUND;
    }
    if (from->usdc_balance < amount)
    {
        return DIN_ERR_INSUFFICIENT_FUNDS;
    }
    from->usdc_balance -= amount;
    to->usdc_balance += amount;
    return DIN_OK;
}

din_wallet_t *din_wallet_find_by_address(din_wallet_manager_t *mgr, const char *addr)
{
    if (!mgr || !addr)
    {
        return NULL;
    }
    for (uint32_t i = 0; i < mgr->count; ++i)
    {
        if (mgr->wallets[i].is_active && strcmp(mgr->wallets[i].address.str, addr) == 0)
        {
            return &mgr->wallets[i];
        }
    }
    return NULL;
}

din_error_t din_wallet_save_file(const din_wallet_manager_t *mgr, const char *filepath)
{
    if (!mgr || !filepath)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    FILE *f = fopen(filepath, "w");
    if (!f)
    {
        return DIN_ERR_INVALID_STATE;
    }
    fprintf(f, "# DIN Sovereign Keystore\n");
    fprintf(f, "count=%u\n", mgr->count);
    for (uint32_t i = 0; i < mgr->count; ++i)
    {
        const din_wallet_t *w = &mgr->wallets[i];
        char priv_hex[DIN_HASH_LEN * 2 + 1];
        din_hash256_t pkh;
        memcpy(pkh.bytes, w->private_key, DIN_HASH_LEN);
        din_hash_to_hex(&pkh, priv_hex);

        fprintf(f, "[wallet_%u]\n", i);
        fprintf(f, "label=%s\n", w->label);
        fprintf(f, "role=%d\n", w->role);
        fprintf(f, "address=%s\n", w->address.str);
        fprintf(f, "did=%s\n", w->did);
        fprintf(f, "mnemonic=%s\n", w->mnemonic);
        fprintf(f, "private_key=%s\n", priv_hex);
        fprintf(f, "usdc_balance=%.2f\n", w->usdc_balance);
        fprintf(f, "eth_balance=%.4f\n", w->eth_balance);
        fprintf(f, "staked_nft=%u\n", w->staked_nft_id);
    }
    fclose(f);
    return DIN_OK;
}

din_error_t din_wallet_load_file(din_wallet_manager_t *mgr, const char *filepath)
{
    if (!mgr || !filepath)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    FILE *f = fopen(filepath, "r");
    if (!f)
    {
        return DIN_ERR_NOT_FOUND;
    }
    char line[256];
    din_wallet_init_manager(mgr);
    din_wallet_t current;
    memset(&current, 0, sizeof(din_wallet_t));
    bool in_wallet = false;

    while (fgets(line, sizeof(line), f))
    {
        line[strcspn(line, "\r\n")] = '\0';
        if (strncmp(line, "[wallet_", 8) == 0)
        {
            if (in_wallet && mgr->count < DIN_MAX_WALLETS)
            {
                mgr->wallets[mgr->count++] = current;
            }
            memset(&current, 0, sizeof(din_wallet_t));
            current.is_active = true;
            in_wallet = true;
        }
        else if (strncmp(line, "label=", 6) == 0)
        {
            snprintf(current.label, DIN_WALLET_LABEL_MAX, "%s", line + 6);
        }
        else if (strncmp(line, "role=", 5) == 0)
        {
            current.role = (din_role_t)atoi(line + 5);
        }
        else if (strncmp(line, "address=", 8) == 0)
        {
            snprintf(current.address.str, DIN_ADDR_MAX_LEN, "%s", line + 8);
        }
        else if (strncmp(line, "did=", 4) == 0)
        {
            snprintf(current.did, DIN_DID_MAX_LEN, "%s", line + 4);
        }
        else if (strncmp(line, "mnemonic=", 9) == 0)
        {
            snprintf(current.mnemonic, DIN_MNEMONIC_MAX_LEN, "%s", line + 9);
        }
        else if (strncmp(line, "usdc_balance=", 13) == 0)
        {
            current.usdc_balance = (float)atof(line + 13);
        }
        else if (strncmp(line, "eth_balance=", 12) == 0)
        {
            current.eth_balance = (float)atof(line + 12);
        }
        else if (strncmp(line, "staked_nft=", 11) == 0)
        {
            current.staked_nft_id = (uint32_t)atoi(line + 11);
        }
    }
    if (in_wallet && mgr->count < DIN_MAX_WALLETS)
    {
        mgr->wallets[mgr->count++] = current;
    }
    fclose(f);
    return DIN_OK;
}
