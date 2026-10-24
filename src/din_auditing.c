#include "din/din_auditing.h"
#include "din/din_crypto.h"
#include <math.h>
#include <string.h>

din_error_t din_auditing_init_control_dataset(din_control_dataset_t *cds, uint32_t feature_dim)
{
    if (!cds || feature_dim == 0 || feature_dim > DIN_MAX_PARAMS)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    cds->num_samples = 0;
    cds->feature_dim = feature_dim;
    cds->is_encrypted = true;
    return DIN_OK;
}

din_error_t din_auditing_add_control_sample(din_control_dataset_t *cds, const float *features, float target)
{
    if (!cds || !features || cds->num_samples >= DIN_MAX_DATASET_SIZE)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    uint32_t idx = cds->num_samples;
    memcpy(cds->features[idx], features, sizeof(float) * cds->feature_dim);
    cds->targets[idx] = target;
    cds->num_samples++;
    return DIN_OK;
}

din_error_t din_auditing_evaluate_model(const din_model_t *model, const din_control_dataset_t *cds, float *out_score)
{
    if (!model || !cds || !out_score || cds->num_samples == 0)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    float total_sq_err = 0.0f;
    for (uint32_t s = 0; s < cds->num_samples; ++s)
    {
        float pred = din_model_predict_linear(model, cds->features[s], cds->feature_dim);
        float diff = pred - cds->targets[s];
        total_sq_err += diff * diff;
    }
    float mse = total_sq_err / (float)cds->num_samples;
    *out_score = 1.0f / (1.0f + mse);
    return DIN_OK;
}

din_error_t din_auditing_commit_score(din_eval_commit_t *commit, uint32_t evaluator_id, uint32_t participant_id, float score, uint32_t salt)
{
    if (!commit)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    commit->evaluator_id = evaluator_id;
    commit->participant_id = participant_id;
    commit->committed = true;
    din_commit_score(score, salt, &commit->commitment);
    return DIN_OK;
}

din_error_t din_auditing_reveal_score(din_eval_reveal_t *reveal, const din_eval_commit_t *commit, float score, uint32_t salt)
{
    if (!reveal || !commit || !commit->committed)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    reveal->evaluator_id = commit->evaluator_id;
    reveal->participant_id = commit->participant_id;
    reveal->score = score;
    reveal->salt = salt;
    reveal->revealed = true;
    reveal->valid = din_verify_score_reveal(score, salt, &commit->commitment);
    return reveal->valid ? DIN_OK : DIN_ERR_CRYPTO_FAIL;
}

float din_auditing_compute_median(float *values, uint32_t count)
{
    if (!values || count == 0)
    {
        return 0.0f;
    }
    for (uint32_t i = 0; i < count - 1; ++i)
    {
        for (uint32_t j = 0; j < count - i - 1; ++j)
        {
            if (values[j] > values[j + 1])
            {
                float tmp = values[j];
                values[j] = values[j + 1];
                values[j + 1] = tmp;
            }
        }
    }
    if (count % 2 == 1)
    {
        return values[count / 2];
    }
    return (values[(count / 2) - 1] + values[count / 2]) * 0.5f;
}

din_error_t din_auditing_process_matrix(din_auditing_matrix_t *matrix, float outlier_threshold)
{
    if (!matrix || matrix->num_participants == 0 || matrix->num_evaluators == 0)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    for (uint32_t e = 0; e < matrix->num_evaluators; ++e)
    {
        matrix->evaluator_slashed[e] = false;
    }

    for (uint32_t p = 0; p < matrix->num_participants; ++p)
    {
        float eval_scores[DIN_MAX_EVALUATORS];
        uint32_t eval_count = 0;

        for (uint32_t e = 0; e < matrix->num_evaluators; ++e)
        {
            if (matrix->has_score[p][e])
            {
                eval_scores[eval_count++] = matrix->raw_scores[p][e];
            }
        }
        if (eval_count == 0)
        {
            matrix->median_scores[p] = 0.0f;
            matrix->final_participant_scores[p] = 0.0f;
            continue;
        }
        float med = din_auditing_compute_median(eval_scores, eval_count);
        matrix->median_scores[p] = med;

        float valid_sum = 0.0f;
        uint32_t valid_count = 0;
        for (uint32_t e = 0; e < matrix->num_evaluators; ++e)
        {
            if (matrix->has_score[p][e])
            {
                float diff = fabsf(matrix->raw_scores[p][e] - med);
                if (diff > outlier_threshold)
                {
                    matrix->filtered_scores[p][e] = 0.0f;
                    matrix->evaluator_slashed[e] = true;
                }
                else
                {
                    matrix->filtered_scores[p][e] = matrix->raw_scores[p][e];
                    valid_sum += matrix->filtered_scores[p][e];
                    valid_count++;
                }
            }
        }
        matrix->final_participant_scores[p] = (valid_count > 0) ? (valid_sum / (float)valid_count) : 0.0f;
    }
    return DIN_OK;
}
