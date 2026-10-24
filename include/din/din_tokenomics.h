#ifndef DIN_TOKENOMICS_H
#define DIN_TOKENOMICS_H

#include "din_types.h"

typedef struct
{
    uint32_t token_id;
    din_addr_t owner_address;
    float self_assessed_valuation;
    float annual_tax_rate;
    float accumulated_tax_owed;
    bool is_slashed;
    bool active;
} din_evaluator_nft_t;

typedef struct
{
    float participant_reward_share;
    float evaluator_reward_share;
    float public_goods_treasury;
    uint32_t num_nfts;
    din_evaluator_nft_t nfts[DIN_MAX_EVALUATORS];
} din_tokenomics_engine_t;

din_error_t din_tokenomics_init(din_tokenomics_engine_t *engine, float p_share, float e_share);
din_error_t din_tokenomics_mint_nft(din_tokenomics_engine_t *engine, uint32_t token_id, const char *owner, float initial_val, float tax_rate);
din_error_t din_tokenomics_reassess_valuation(din_tokenomics_engine_t *engine, uint32_t token_id, const char *caller, float new_val);
din_error_t din_tokenomics_apply_harberger_tax(din_tokenomics_engine_t *engine, uint32_t token_id, float epoch_fraction, float *out_tax_charged);
din_error_t din_tokenomics_pco_forced_sale(din_tokenomics_engine_t *engine, uint32_t token_id, const char *buyer, float payment, float new_self_val);
din_error_t din_tokenomics_slash_evaluator(din_tokenomics_engine_t *engine, uint32_t token_id);
din_error_t din_tokenomics_calculate_payouts(din_tokenomics_engine_t *engine, float total_escrow, const float *participant_scores, uint32_t p_count, float *out_p_rewards, const uint32_t *evaluator_ids, uint32_t e_count, float *out_e_rewards);

#endif
