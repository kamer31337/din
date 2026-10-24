#include "din/din_tokenomics.h"
#include <stdio.h>
#include <string.h>

din_error_t din_tokenomics_init(din_tokenomics_engine_t *engine, float p_share, float e_share)
{
    if (!engine || p_share < 0.0f || e_share < 0.0f || (p_share + e_share > 1.0001f))
    {
        return DIN_ERR_INVALID_PARAM;
    }
    engine->participant_reward_share = p_share;
    engine->evaluator_reward_share = e_share;
    engine->public_goods_treasury = 0.0f;
    engine->num_nfts = 0;
    for (uint32_t i = 0; i < DIN_MAX_EVALUATORS; ++i)
    {
        engine->nfts[i].active = false;
        engine->nfts[i].is_slashed = false;
    }
    return DIN_OK;
}

din_error_t din_tokenomics_mint_nft(din_tokenomics_engine_t *engine, uint32_t token_id, const char *owner, float initial_val, float tax_rate)
{
    if (!engine || !owner || initial_val <= 0.0f || tax_rate <= 0.0f || engine->num_nfts >= DIN_MAX_EVALUATORS)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    uint32_t idx = engine->num_nfts;
    engine->nfts[idx].token_id = token_id;
    engine->nfts[idx].self_assessed_valuation = initial_val;
    engine->nfts[idx].annual_tax_rate = tax_rate;
    engine->nfts[idx].accumulated_tax_owed = 0.0f;
    engine->nfts[idx].is_slashed = false;
    engine->nfts[idx].active = true;
    snprintf(engine->nfts[idx].owner_address.str, DIN_ADDR_MAX_LEN, "%s", owner);
    engine->num_nfts++;
    return DIN_OK;
}

din_error_t din_tokenomics_reassess_valuation(din_tokenomics_engine_t *engine, uint32_t token_id, const char *caller, float new_val)
{
    if (!engine || !caller || new_val <= 0.0f)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    for (uint32_t i = 0; i < engine->num_nfts; ++i)
    {
        if (engine->nfts[i].token_id == token_id && engine->nfts[i].active)
        {
            if (strcmp(engine->nfts[i].owner_address.str, caller) != 0)
            {
                return DIN_ERR_UNAUTHORIZED;
            }
            engine->nfts[i].self_assessed_valuation = new_val;
            return DIN_OK;
        }
    }
    return DIN_ERR_NOT_FOUND;
}

din_error_t din_tokenomics_apply_harberger_tax(din_tokenomics_engine_t *engine, uint32_t token_id, float epoch_fraction, float *out_tax_charged)
{
    if (!engine || epoch_fraction <= 0.0f || !out_tax_charged)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    for (uint32_t i = 0; i < engine->num_nfts; ++i)
    {
        if (engine->nfts[i].token_id == token_id && engine->nfts[i].active && !engine->nfts[i].is_slashed)
        {
            float tax = engine->nfts[i].self_assessed_valuation * engine->nfts[i].annual_tax_rate * epoch_fraction;
            engine->nfts[i].accumulated_tax_owed += tax;
            engine->public_goods_treasury += tax;
            *out_tax_charged = tax;
            return DIN_OK;
        }
    }
    return DIN_ERR_NOT_FOUND;
}

din_error_t din_tokenomics_pco_forced_sale(din_tokenomics_engine_t *engine, uint32_t token_id, const char *buyer, float payment, float new_self_val)
{
    if (!engine || !buyer || payment <= 0.0f || new_self_val <= 0.0f)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    for (uint32_t i = 0; i < engine->num_nfts; ++i)
    {
        if (engine->nfts[i].token_id == token_id && engine->nfts[i].active && !engine->nfts[i].is_slashed)
        {
            if (payment < engine->nfts[i].self_assessed_valuation)
            {
                return DIN_ERR_INSUFFICIENT_FUNDS;
            }
            snprintf(engine->nfts[i].owner_address.str, DIN_ADDR_MAX_LEN, "%s", buyer);
            engine->nfts[i].self_assessed_valuation = new_self_val;
            return DIN_OK;
        }
    }
    return DIN_ERR_NOT_FOUND;
}

din_error_t din_tokenomics_slash_evaluator(din_tokenomics_engine_t *engine, uint32_t token_id)
{
    if (!engine)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    for (uint32_t i = 0; i < engine->num_nfts; ++i)
    {
        if (engine->nfts[i].token_id == token_id)
        {
            engine->nfts[i].is_slashed = true;
            engine->nfts[i].active = false;
            engine->public_goods_treasury += engine->nfts[i].self_assessed_valuation * 0.5f;
            return DIN_OK;
        }
    }
    return DIN_ERR_NOT_FOUND;
}

din_error_t din_tokenomics_calculate_payouts(din_tokenomics_engine_t *engine, float total_escrow, const float *participant_scores, uint32_t p_count, float *out_p_rewards, const uint32_t *evaluator_ids, uint32_t e_count, float *out_e_rewards)
{
    if (!engine || total_escrow <= 0.0f || !participant_scores || !out_p_rewards || !out_e_rewards)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    float p_pool = total_escrow * engine->participant_reward_share;
    float e_pool = total_escrow * engine->evaluator_reward_share;
    float score_sum = 0.0f;

    for (uint32_t p = 0; p < p_count; ++p)
    {
        if (participant_scores[p] > 0.0f)
        {
            score_sum += participant_scores[p];
        }
    }
    for (uint32_t p = 0; p < p_count; ++p)
    {
        if (score_sum > 0.0f && participant_scores[p] > 0.0f)
        {
            out_p_rewards[p] = p_pool * (participant_scores[p] / score_sum);
        }
        else
        {
            out_p_rewards[p] = 0.0f;
        }
    }
    uint32_t honest_eval_count = 0;
    for (uint32_t e = 0; e < e_count; ++e)
    {
        uint32_t tid = evaluator_ids[e];
        bool honest = true;
        for (uint32_t i = 0; i < engine->num_nfts; ++i)
        {
            if (engine->nfts[i].token_id == tid && engine->nfts[i].is_slashed)
            {
                honest = false;
                break;
            }
        }
        if (honest)
        {
            honest_eval_count++;
        }
    }
    for (uint32_t e = 0; e < e_count; ++e)
    {
        uint32_t tid = evaluator_ids[e];
        bool honest = true;
        for (uint32_t i = 0; i < engine->num_nfts; ++i)
        {
            if (engine->nfts[i].token_id == tid && engine->nfts[i].is_slashed)
            {
                honest = false;
                break;
            }
        }
        if (honest && honest_eval_count > 0)
        {
            out_e_rewards[e] = e_pool / (float)honest_eval_count;
        }
        else
        {
            out_e_rewards[e] = 0.0f;
        }
    }
    return DIN_OK;
}
