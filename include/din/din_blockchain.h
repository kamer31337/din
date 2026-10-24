#ifndef DIN_BLOCKCHAIN_H
#define DIN_BLOCKCHAIN_H

#include "din_types.h"

typedef struct
{
    uint32_t participant_id;
    din_addr_t address;
    din_cid_t model_cid;
    bool submitted;
    float final_score;
    float reward_payout;
} din_participant_record_t;

typedef struct
{
    uint32_t evaluator_id;
    din_addr_t address;
    uint32_t nft_token_id;
    float staked_valuation;
    float accumulated_tax;
    float reward_payout;
    bool active;
    bool slashed;
} din_evaluator_record_t;

typedef struct
{
    din_phase_t current_phase;
    uint32_t current_round;
    din_addr_t model_owner;
    din_cid_t genesis_cid;
    din_cid_t current_global_cid;
    float reward_escrow_pool;
    float public_goods_treasury;
    uint32_t num_participants;
    din_participant_record_t participants[DIN_MAX_PARTICIPANTS];
    uint32_t num_evaluators;
    din_evaluator_record_t evaluators[DIN_MAX_EVALUATORS];
} din_blockchain_t;

din_error_t din_blockchain_init(din_blockchain_t *chain, const char *model_owner_addr);
din_error_t din_blockchain_deposit_escrow(din_blockchain_t *chain, float amount);
din_error_t din_blockchain_register_genesis(din_blockchain_t *chain, const din_cid_t *genesis_cid);
din_error_t din_blockchain_register_participant(din_blockchain_t *chain, uint32_t participant_id, const char *address);
din_error_t din_blockchain_submit_participant_cid(din_blockchain_t *chain, uint32_t participant_id, const din_cid_t *cid);
din_error_t din_blockchain_update_global_model(din_blockchain_t *chain, const din_cid_t *new_global_cid);
din_error_t din_blockchain_start_next_round(din_blockchain_t *chain);
din_error_t din_blockchain_transition_to_auditing(din_blockchain_t *chain);
din_error_t din_blockchain_distribute_rewards(din_blockchain_t *chain, float participant_ratio, float evaluator_ratio);
din_phase_t din_blockchain_get_phase(const din_blockchain_t *chain);

#endif
