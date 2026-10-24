#include "din/din_secagg.h"
#include "din/din_crypto.h"
#include <string.h>

din_error_t din_secagg_init(din_hierarchical_aggregator_t *hagg, uint32_t k_threshold)
{
    if (!hagg || k_threshold == 0)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    hagg->num_subgroups = 0;
    hagg->k_threshold = k_threshold;
    din_model_init(&hagg->final_aggregated_delta, DIN_MAX_PARAMS);
    din_model_init(&hagg->new_global_model, DIN_MAX_PARAMS);
    return DIN_OK;
}

din_error_t din_secagg_assign_subgroups(din_hierarchical_aggregator_t *hagg, const uint32_t *participant_ids, uint32_t total_participants, uint32_t devices_per_subgroup)
{
    if (!hagg || !participant_ids || total_participants == 0 || devices_per_subgroup == 0)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    if (devices_per_subgroup < hagg->k_threshold)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    hagg->num_subgroups = (total_participants + devices_per_subgroup - 1) / devices_per_subgroup;
    if (hagg->num_subgroups > DIN_MAX_SUBGROUPS)
    {
        return DIN_ERR_OUT_OF_MEMORY;
    }
    uint32_t cursor = 0;
    for (uint32_t g = 0; g < hagg->num_subgroups; ++g)
    {
        din_subgroup_aggregator_t *sg = &hagg->subgroups[g];
        sg->subgroup_id = g;
        sg->k_min_devices = hagg->k_threshold;
        sg->num_participants = 0;
        din_model_init(&sg->intermediate_sum, DIN_MAX_PARAMS);
        din_model_init(&sg->intermediate_mean, DIN_MAX_PARAMS);

        for (uint32_t i = 0; i < devices_per_subgroup && cursor < total_participants; ++i)
        {
            sg->participant_ids[sg->num_participants++] = participant_ids[cursor++];
        }
    }
    return DIN_OK;
}

din_error_t din_secagg_mask_participant_delta(uint32_t participant_id, const uint32_t *subgroup_members, uint32_t member_count, uint32_t round_num, const din_model_t *raw_delta, din_model_t *out_masked_delta)
{
    if (!subgroup_members || member_count == 0 || !raw_delta || !out_masked_delta)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    din_model_copy(out_masked_delta, raw_delta);
    float pairwise_mask[DIN_MAX_PARAMS];

    for (uint32_t idx = 0; idx < member_count; ++idx)
    {
        uint32_t other_id = subgroup_members[idx];
        if (other_id != participant_id)
        {
            din_generate_pairwise_mask(participant_id, other_id, round_num, pairwise_mask, raw_delta->num_params, 1.0f);
            for (uint32_t p = 0; p < raw_delta->num_params; ++p)
            {
                out_masked_delta->weights[p] += pairwise_mask[p];
            }
        }
    }
    return DIN_OK;
}

din_error_t din_secagg_subgroup_aggregate(din_subgroup_aggregator_t *subgroup, const din_model_t *masked_deltas, uint32_t count)
{
    if (!subgroup || !masked_deltas || count == 0)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    if (count < subgroup->k_min_devices)
    {
        return DIN_ERR_BYZANTINE_DETECTED;
    }
    uint32_t num_params = masked_deltas[0].num_params;
    din_model_init(&subgroup->intermediate_sum, num_params);
    din_model_init(&subgroup->intermediate_mean, num_params);

    for (uint32_t i = 0; i < count; ++i)
    {
        din_model_add(&subgroup->intermediate_sum, &masked_deltas[i]);
    }
    din_model_copy(&subgroup->intermediate_mean, &subgroup->intermediate_sum);
    din_model_scale(&subgroup->intermediate_mean, 1.0f / (float)count);
    return DIN_OK;
}

din_error_t din_secagg_master_merge(din_hierarchical_aggregator_t *hagg, const din_model_t *current_global, din_model_t *out_next_global)
{
    if (!hagg || !current_global || !out_next_global || hagg->num_subgroups == 0)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    uint32_t num_params = current_global->num_params;
    din_model_init(&hagg->final_aggregated_delta, num_params);
    uint32_t total_devices = 0;

    for (uint32_t g = 0; g < hagg->num_subgroups; ++g)
    {
        total_devices += hagg->subgroups[g].num_participants;
    }
    if (total_devices == 0)
    {
        return DIN_ERR_INVALID_STATE;
    }
    for (uint32_t g = 0; g < hagg->num_subgroups; ++g)
    {
        din_subgroup_aggregator_t *sg = &hagg->subgroups[g];
        float weight = (float)sg->num_participants / (float)total_devices;
        for (uint32_t p = 0; p < num_params; ++p)
        {
            hagg->final_aggregated_delta.weights[p] += sg->intermediate_mean.weights[p] * weight;
        }
    }
    din_model_copy(out_next_global, current_global);
    din_model_apply_delta(out_next_global, &hagg->final_aggregated_delta);
    out_next_global->round = current_global->round + 1;
    din_model_copy(&hagg->new_global_model, out_next_global);
    return DIN_OK;
}
