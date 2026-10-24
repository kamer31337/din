#include "din/din_blockchain.h"
#include <stdio.h>
#include <string.h>

din_error_t din_blockchain_init(din_blockchain_t *chain, const char *model_owner_addr)
{
    if (!chain || !model_owner_addr)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    chain->current_phase = DIN_PHASE_GENESIS;
    chain->current_round = 0;
    chain->reward_escrow_pool = 0.0f;
    chain->public_goods_treasury = 0.0f;
    chain->num_participants = 0;
    chain->num_evaluators = 0;
    chain->genesis_cid.str[0] = '\0';
    chain->current_global_cid.str[0] = '\0';
    snprintf(chain->model_owner.str, DIN_ADDR_MAX_LEN, "%s", model_owner_addr);
    return DIN_OK;
}

din_error_t din_blockchain_deposit_escrow(din_blockchain_t *chain, float amount)
{
    if (!chain || amount <= 0.0f)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    chain->reward_escrow_pool += amount;
    return DIN_OK;
}

din_error_t din_blockchain_register_genesis(din_blockchain_t *chain, const din_cid_t *genesis_cid)
{
    if (!chain || !genesis_cid || chain->current_phase != DIN_PHASE_GENESIS)
    {
        return DIN_ERR_INVALID_STATE;
    }
    chain->genesis_cid = *genesis_cid;
    chain->current_global_cid = *genesis_cid;
    chain->current_round = 1;
    chain->current_phase = DIN_PHASE_TRAINING;
    return DIN_OK;
}

din_error_t din_blockchain_register_participant(din_blockchain_t *chain, uint32_t participant_id, const char *address)
{
    if (!chain || !address || chain->num_participants >= DIN_MAX_PARTICIPANTS)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    for (uint32_t i = 0; i < chain->num_participants; ++i)
    {
        if (chain->participants[i].participant_id == participant_id)
        {
            return DIN_OK;
        }
    }
    uint32_t idx = chain->num_participants;
    chain->participants[idx].participant_id = participant_id;
    chain->participants[idx].submitted = false;
    chain->participants[idx].final_score = 0.0f;
    chain->participants[idx].reward_payout = 0.0f;
    chain->participants[idx].model_cid.str[0] = '\0';
    snprintf(chain->participants[idx].address.str, DIN_ADDR_MAX_LEN, "%s", address);
    chain->num_participants++;
    return DIN_OK;
}

din_error_t din_blockchain_submit_participant_cid(din_blockchain_t *chain, uint32_t participant_id, const din_cid_t *cid)
{
    if (!chain || !cid || chain->current_phase != DIN_PHASE_TRAINING)
    {
        return DIN_ERR_INVALID_STATE;
    }
    for (uint32_t i = 0; i < chain->num_participants; ++i)
    {
        if (chain->participants[i].participant_id == participant_id)
        {
            chain->participants[i].model_cid = *cid;
            chain->participants[i].submitted = true;
            return DIN_OK;
        }
    }
    return DIN_ERR_NOT_FOUND;
}

din_error_t din_blockchain_update_global_model(din_blockchain_t *chain, const din_cid_t *new_global_cid)
{
    if (!chain || !new_global_cid)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    chain->current_global_cid = *new_global_cid;
    chain->current_phase = DIN_PHASE_AGGREGATION;
    return DIN_OK;
}

din_error_t din_blockchain_start_next_round(din_blockchain_t *chain)
{
    if (!chain)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    chain->current_round++;
    chain->current_phase = DIN_PHASE_TRAINING;
    for (uint32_t i = 0; i < chain->num_participants; ++i)
    {
        chain->participants[i].submitted = false;
    }
    return DIN_OK;
}

din_error_t din_blockchain_transition_to_auditing(din_blockchain_t *chain)
{
    if (!chain)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    chain->current_phase = DIN_PHASE_AUDITING;
    return DIN_OK;
}

din_error_t din_blockchain_distribute_rewards(din_blockchain_t *chain, float participant_ratio, float evaluator_ratio)
{
    if (!chain || chain->current_phase != DIN_PHASE_AUDITING)
    {
        return DIN_ERR_INVALID_STATE;
    }
    float participant_pool = chain->reward_escrow_pool * participant_ratio;
    float evaluator_pool = chain->reward_escrow_pool * evaluator_ratio;
    float sum_scores = 0.0f;

    for (uint32_t i = 0; i < chain->num_participants; ++i)
    {
        if (chain->participants[i].final_score > 0.0f)
        {
            sum_scores += chain->participants[i].final_score;
        }
    }
    if (sum_scores > 0.0f)
    {
        for (uint32_t i = 0; i < chain->num_participants; ++i)
        {
            if (chain->participants[i].final_score > 0.0f)
            {
                float share = chain->participants[i].final_score / sum_scores;
                chain->participants[i].reward_payout = participant_pool * share;
            }
            else
            {
                chain->participants[i].reward_payout = 0.0f;
            }
        }
    }
    uint32_t active_evals = 0;
    for (uint32_t j = 0; j < chain->num_evaluators; ++j)
    {
        if (chain->evaluators[j].active && !chain->evaluators[j].slashed)
        {
            active_evals++;
        }
    }
    if (active_evals > 0)
    {
        float eval_share = evaluator_pool / (float)active_evals;
        for (uint32_t j = 0; j < chain->num_evaluators; ++j)
        {
            if (chain->evaluators[j].active && !chain->evaluators[j].slashed)
            {
                chain->evaluators[j].reward_payout = eval_share;
            }
            else
            {
                chain->evaluators[j].reward_payout = 0.0f;
            }
        }
    }
    chain->reward_escrow_pool = 0.0f;
    chain->current_phase = DIN_PHASE_REWARD_SETTLED;
    return DIN_OK;
}

din_phase_t din_blockchain_get_phase(const din_blockchain_t *chain)
{
    if (!chain)
    {
        return DIN_PHASE_UNINITIALIZED;
    }
    return chain->current_phase;
}
