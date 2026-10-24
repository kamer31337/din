#include "din/din_coordinator.h"
#include <stdio.h>
#include <string.h>

din_error_t din_coordinator_init(din_coordinator_t *coord, const char *model_owner_addr, float escrow_amount, uint32_t num_features)
{
    if (!coord || !model_owner_addr || escrow_amount <= 0.0f || num_features == 0)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    memset(coord, 0, sizeof(din_coordinator_t));
    coord->num_participants = 0;
    coord->num_evaluators = 0;
    coord->k_secagg_threshold = 3;
    coord->outlier_threshold = 0.5f;

    din_blockchain_init(&coord->chain, model_owner_addr);
    din_blockchain_deposit_escrow(&coord->chain, escrow_amount);
    din_ipfs_init(&coord->ipfs);
    din_secagg_init(&coord->agg, coord->k_secagg_threshold);
    din_tokenomics_init(&coord->tokenomics, 0.97f, 0.03f);
    din_auditing_init_control_dataset(&coord->control_dataset, num_features);
    din_wallet_init_manager(&coord->wallet_mgr);
    din_model_init(&coord->current_model, num_features);
    return DIN_OK;
}

din_error_t din_coordinator_register_participant(din_coordinator_t *coord, uint32_t p_id, const char *address)
{
    if (!coord || !address || coord->num_participants >= DIN_MAX_PARTICIPANTS)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    uint32_t idx = coord->num_participants;
    din_pds_init(&coord->participants[idx], p_id, address);
    din_blockchain_register_participant(&coord->chain, p_id, address);
    coord->num_participants++;
    return DIN_OK;
}

din_error_t din_coordinator_register_evaluator(din_coordinator_t *coord, uint32_t e_id, const char *address, float valuation, float tax_rate)
{
    if (!coord || !address || coord->num_evaluators >= DIN_MAX_EVALUATORS)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    uint32_t idx = coord->num_evaluators;
    coord->chain.evaluators[idx].evaluator_id = e_id;
    coord->chain.evaluators[idx].nft_token_id = 1000 + e_id;
    coord->chain.evaluators[idx].staked_valuation = valuation;
    coord->chain.evaluators[idx].accumulated_tax = 0.0f;
    coord->chain.evaluators[idx].reward_payout = 0.0f;
    coord->chain.evaluators[idx].active = true;
    coord->chain.evaluators[idx].slashed = false;
    snprintf(coord->chain.evaluators[idx].address.str, DIN_ADDR_MAX_LEN, "%s", address);
    din_tokenomics_mint_nft(&coord->tokenomics, 1000 + e_id, address, valuation, tax_rate);
    coord->chain.num_evaluators++;
    coord->num_evaluators++;
    return DIN_OK;
}

din_error_t din_coordinator_setup_genesis_model(din_coordinator_t *coord, const din_model_t *initial_model)
{
    if (!coord || !initial_model)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    din_cid_t genesis_cid;
    din_model_copy(&coord->current_model, initial_model);
    din_ipfs_put_model(&coord->ipfs, &coord->current_model, &genesis_cid);
    din_blockchain_register_genesis(&coord->chain, &genesis_cid);
    return DIN_OK;
}

din_error_t din_coordinator_run_training_round(din_coordinator_t *coord, float learning_rate, uint32_t local_epochs)
{
    if (!coord || coord->num_participants == 0)
    {
        return DIN_ERR_INVALID_STATE;
    }
    uint32_t p_ids[DIN_MAX_PARTICIPANTS];
    din_model_t updated_models[DIN_MAX_PARTICIPANTS];
    din_model_t raw_deltas[DIN_MAX_PARTICIPANTS];
    din_model_t masked_deltas[DIN_MAX_PARTICIPANTS];
    din_cid_t uploaded_cids[DIN_MAX_PARTICIPANTS];

    for (uint32_t i = 0; i < coord->num_participants; ++i)
    {
        p_ids[i] = coord->participants[i].participant_id;
        din_pds_train_local_step(&coord->participants[i], &coord->current_model, &updated_models[i], learning_rate, local_epochs);
        din_pds_compute_model_delta(&coord->current_model, &updated_models[i], &raw_deltas[i]);
    }
    uint32_t devices_per_group = (coord->num_participants > 8) ? 5 : coord->num_participants;
    if (devices_per_group < coord->k_secagg_threshold)
    {
        devices_per_group = coord->k_secagg_threshold;
    }
    din_secagg_assign_subgroups(&coord->agg, p_ids, coord->num_participants, devices_per_group);

    for (uint32_t g = 0; g < coord->agg.num_subgroups; ++g)
    {
        din_subgroup_aggregator_t *sg = &coord->agg.subgroups[g];
        din_model_t sg_masked[DIN_MAX_PARTICIPANTS];

        for (uint32_t m = 0; m < sg->num_participants; ++m)
        {
            uint32_t pid = sg->participant_ids[m];
            uint32_t p_idx = 0;
            for (uint32_t i = 0; i < coord->num_participants; ++i)
            {
                if (coord->participants[i].participant_id == pid)
                {
                    p_idx = i;
                    break;
                }
            }
            din_secagg_mask_participant_delta(pid, sg->participant_ids, sg->num_participants, coord->chain.current_round, &raw_deltas[p_idx], &sg_masked[m]);
            masked_deltas[p_idx] = sg_masked[m];
            din_ipfs_put_model(&coord->ipfs, &masked_deltas[p_idx], &uploaded_cids[p_idx]);
            din_blockchain_submit_participant_cid(&coord->chain, pid, &uploaded_cids[p_idx]);
        }
        din_secagg_subgroup_aggregate(sg, sg_masked, sg->num_participants);
    }
    din_model_t next_global;
    din_secagg_master_merge(&coord->agg, &coord->current_model, &next_global);
    din_model_copy(&coord->current_model, &next_global);

    din_cid_t next_global_cid;
    din_ipfs_put_model(&coord->ipfs, &coord->current_model, &next_global_cid);
    din_blockchain_update_global_model(&coord->chain, &next_global_cid);
    din_blockchain_start_next_round(&coord->chain);
    return DIN_OK;
}

din_error_t din_coordinator_run_auditing_and_settlement(din_coordinator_t *coord)
{
    if (!coord || coord->num_participants == 0 || coord->num_evaluators == 0)
    {
        return DIN_ERR_INVALID_STATE;
    }
    din_blockchain_transition_to_auditing(&coord->chain);
    din_auditing_matrix_t matrix;
    matrix.num_participants = coord->num_participants;
    matrix.num_evaluators = coord->num_evaluators;
    memset(matrix.has_score, 0, sizeof(matrix.has_score));

    din_eval_commit_t commits[DIN_MAX_PARTICIPANTS][DIN_MAX_EVALUATORS];
    din_eval_reveal_t reveals[DIN_MAX_PARTICIPANTS][DIN_MAX_EVALUATORS];

    for (uint32_t p = 0; p < coord->num_participants; ++p)
    {
        din_model_t p_model;
        din_model_copy(&p_model, &coord->current_model);
        if (coord->participants[p].is_byzantine)
        {
            for (uint32_t k = 0; k < p_model.num_params; ++k)
            {
                p_model.weights[k] += 25.0f;
            }
        }
        for (uint32_t e = 0; e < coord->num_evaluators; ++e)
        {
            float eval_score = 0.0f;
            din_auditing_evaluate_model(&p_model, &coord->control_dataset, &eval_score);
            if (e == coord->num_evaluators - 1 && coord->num_evaluators > 2)
            {
                eval_score = (eval_score > 0.5f) ? 0.05f : 0.95f;
            }
            uint32_t salt = 77700u + (p * 31) + (e * 17);
            din_auditing_commit_score(&commits[p][e], e, p, eval_score, salt);
            din_error_t rev_err = din_auditing_reveal_score(&reveals[p][e], &commits[p][e], eval_score, salt);
            if (rev_err == DIN_OK)
            {
                matrix.raw_scores[p][e] = eval_score;
                matrix.has_score[p][e] = true;
            }
        }
    }
    din_auditing_process_matrix(&matrix, coord->outlier_threshold);

    for (uint32_t p = 0; p < coord->num_participants; ++p)
    {
        coord->chain.participants[p].final_score = matrix.final_participant_scores[p];
    }
    for (uint32_t e = 0; e < coord->num_evaluators; ++e)
    {
        if (matrix.evaluator_slashed[e])
        {
            coord->chain.evaluators[e].slashed = true;
            din_tokenomics_slash_evaluator(&coord->tokenomics, coord->chain.evaluators[e].nft_token_id);
        }
        else
        {
            float tax_charged = 0.0f;
            din_tokenomics_apply_harberger_tax(&coord->tokenomics, coord->chain.evaluators[e].nft_token_id, 0.05f, &tax_charged);
            coord->chain.evaluators[e].accumulated_tax += tax_charged;
        }
    }
    float p_scores[DIN_MAX_PARTICIPANTS];
    float p_rewards[DIN_MAX_PARTICIPANTS];
    uint32_t e_token_ids[DIN_MAX_EVALUATORS];
    float e_rewards[DIN_MAX_EVALUATORS];

    for (uint32_t p = 0; p < coord->num_participants; ++p)
    {
        p_scores[p] = coord->chain.participants[p].final_score;
    }
    for (uint32_t e = 0; e < coord->num_evaluators; ++e)
    {
        e_token_ids[e] = coord->chain.evaluators[e].nft_token_id;
    }
    din_tokenomics_calculate_payouts(&coord->tokenomics, coord->chain.reward_escrow_pool, p_scores, coord->num_participants, p_rewards, e_token_ids, coord->num_evaluators, e_rewards);

    for (uint32_t p = 0; p < coord->num_participants; ++p)
    {
        coord->chain.participants[p].reward_payout = p_rewards[p];
    }
    for (uint32_t e = 0; e < coord->num_evaluators; ++e)
    {
        coord->chain.evaluators[e].reward_payout = e_rewards[e];
    }
    din_blockchain_distribute_rewards(&coord->chain, coord->tokenomics.participant_reward_share, coord->tokenomics.evaluator_reward_share);
    coord->chain.public_goods_treasury = coord->tokenomics.public_goods_treasury;
    coord->chain.current_phase = DIN_PHASE_COMPLETED;
    return DIN_OK;
}

void din_coordinator_print_summary(const din_coordinator_t *coord)
{
    if (!coord)
    {
        return;
    }
    printf("\n==================================================================\n");
    printf("        DECENTRALIZED INTELLIGENCE NETWORK (DIN) SUMMARY\n");
    printf("==================================================================\n");
    printf("Model Owner Address    : %s\n", coord->chain.model_owner.str);
    printf("Current Global Model CID: %s\n", coord->chain.current_global_cid.str);
    printf("Total Rounds Completed : %u\n", coord->chain.current_round - 1);
    printf("Public Goods Treasury  : $%.2f USDC (from Harberger taxes & slashes)\n", coord->chain.public_goods_treasury);
    printf("------------------------------------------------------------------\n");
    printf("PARTICIPANTS REWARDS (97%% Pool Allocation):\n");
    for (uint32_t p = 0; p < coord->num_participants; ++p)
    {
        const din_participant_record_t *pr = &coord->chain.participants[p];
        printf("  [PDS #%02u] Score: %.4f | Payout: $%.2f USDC | Byz: %s\n",
               pr->participant_id, pr->final_score, pr->reward_payout,
               coord->participants[p].is_byzantine ? "YES (Attacker)" : "NO (Honest)");
    }
    printf("------------------------------------------------------------------\n");
    printf("EVALUATOR AUDITORS (3%% Pool Allocation & Harberger Staking):\n");
    for (uint32_t e = 0; e < coord->num_evaluators; ++e)
    {
        const din_evaluator_record_t *er = &coord->chain.evaluators[e];
        printf("  [NFT #%u] Addr: %s | Val: $%.1f | Tax: $%.2f | Payout: $%.2f | Status: %s\n",
               er->nft_token_id, er->address.str, er->staked_valuation,
               er->accumulated_tax, er->reward_payout,
               er->slashed ? "SLASHED (Byzantine Outlier)" : "HONEST (Active)");
    }
    printf("==================================================================\n\n");
}
