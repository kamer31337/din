#ifndef DIN_SECAGG_H
#define DIN_SECAGG_H

#include "din_types.h"
#include "din_model.h"

typedef struct
{
    uint32_t subgroup_id;
    uint32_t participant_ids[DIN_MAX_PARTICIPANTS];
    uint32_t num_participants;
    uint32_t k_min_devices;
    din_model_t intermediate_sum;
    din_model_t intermediate_mean;
} din_subgroup_aggregator_t;

typedef struct
{
    uint32_t num_subgroups;
    uint32_t k_threshold;
    din_subgroup_aggregator_t subgroups[DIN_MAX_SUBGROUPS];
    din_model_t final_aggregated_delta;
    din_model_t new_global_model;
} din_hierarchical_aggregator_t;

din_error_t din_secagg_init(din_hierarchical_aggregator_t *hagg, uint32_t k_threshold);
din_error_t din_secagg_assign_subgroups(din_hierarchical_aggregator_t *hagg, const uint32_t *participant_ids, uint32_t total_participants, uint32_t devices_per_subgroup);
din_error_t din_secagg_mask_participant_delta(uint32_t participant_id, const uint32_t *subgroup_members, uint32_t member_count, uint32_t round_num, const din_model_t *raw_delta, din_model_t *out_masked_delta);
din_error_t din_secagg_subgroup_aggregate(din_subgroup_aggregator_t *subgroup, const din_model_t *masked_deltas, uint32_t count);
din_error_t din_secagg_master_merge(din_hierarchical_aggregator_t *hagg, const din_model_t *current_global, din_model_t *out_next_global);

#endif
