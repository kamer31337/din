#ifndef DIN_AUDITING_H
#define DIN_AUDITING_H

#include "din_types.h"
#include "din_model.h"

typedef struct
{
    uint32_t num_samples;
    uint32_t feature_dim;
    float features[DIN_MAX_DATASET_SIZE][DIN_MAX_PARAMS];
    float targets[DIN_MAX_DATASET_SIZE];
    bool is_encrypted;
} din_control_dataset_t;

typedef struct
{
    uint32_t evaluator_id;
    uint32_t participant_id;
    din_hash256_t commitment;
    bool committed;
} din_eval_commit_t;

typedef struct
{
    uint32_t evaluator_id;
    uint32_t participant_id;
    float score;
    uint32_t salt;
    bool revealed;
    bool valid;
} din_eval_reveal_t;

typedef struct
{
    uint32_t num_participants;
    uint32_t num_evaluators;
    float raw_scores[DIN_MAX_PARTICIPANTS][DIN_MAX_EVALUATORS];
    bool has_score[DIN_MAX_PARTICIPANTS][DIN_MAX_EVALUATORS];
    float median_scores[DIN_MAX_PARTICIPANTS];
    float filtered_scores[DIN_MAX_PARTICIPANTS][DIN_MAX_EVALUATORS];
    float final_participant_scores[DIN_MAX_PARTICIPANTS];
    bool evaluator_slashed[DIN_MAX_EVALUATORS];
} din_auditing_matrix_t;

din_error_t din_auditing_init_control_dataset(din_control_dataset_t *cds, uint32_t feature_dim);
din_error_t din_auditing_add_control_sample(din_control_dataset_t *cds, const float *features, float target);
din_error_t din_auditing_evaluate_model(const din_model_t *model, const din_control_dataset_t *cds, float *out_score);
din_error_t din_auditing_commit_score(din_eval_commit_t *commit, uint32_t evaluator_id, uint32_t participant_id, float score, uint32_t salt);
din_error_t din_auditing_reveal_score(din_eval_reveal_t *reveal, const din_eval_commit_t *commit, float score, uint32_t salt);
float din_auditing_compute_median(float *values, uint32_t count);
din_error_t din_auditing_process_matrix(din_auditing_matrix_t *matrix, float outlier_threshold);

#endif
