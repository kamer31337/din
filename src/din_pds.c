#include "din/din_pds.h"
#include <stdio.h>
#include <string.h>

din_error_t din_pds_init(din_pds_t *pds, uint32_t participant_id, const char *address)
{
    if (!pds || !address)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    pds->participant_id = participant_id;
    pds->num_samples = 0;
    pds->feature_dim = 0;
    pds->is_byzantine = false;
    snprintf(pds->address.str, DIN_ADDR_MAX_LEN, "%s", address);
    return DIN_OK;
}

din_error_t din_pds_add_sample(din_pds_t *pds, const float *features, uint32_t feature_dim, float target)
{
    if (!pds || !features || feature_dim == 0 || feature_dim > DIN_MAX_PARAMS)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    if (pds->num_samples >= DIN_MAX_DATASET_SIZE)
    {
        return DIN_ERR_OUT_OF_MEMORY;
    }
    pds->feature_dim = feature_dim;
    uint32_t idx = pds->num_samples;
    memcpy(pds->features[idx], features, sizeof(float) * feature_dim);
    pds->targets[idx] = target;
    pds->num_samples++;
    return DIN_OK;
}

din_error_t din_pds_train_local_step(din_pds_t *pds, const din_model_t *global_model, din_model_t *out_updated_model, float learning_rate, uint32_t epochs)
{
    if (!pds || !global_model || !out_updated_model || pds->num_samples == 0)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    din_model_copy(out_updated_model, global_model);
    uint32_t d = global_model->num_params;
    float total_loss = 0.0f;

    for (uint32_t ep = 0; ep < epochs; ++ep)
    {
        total_loss = 0.0f;
        for (uint32_t s = 0; s < pds->num_samples; ++s)
        {
            float pred = 0.0f;
            for (uint32_t j = 0; j < d; ++j)
            {
                pred += out_updated_model->weights[j] * pds->features[s][j];
            }
            float err = pred - pds->targets[s];
            total_loss += err * err;
            for (uint32_t j = 0; j < d; ++j)
            {
                float grad = err * pds->features[s][j];
                if (pds->is_byzantine)
                {
                    out_updated_model->weights[j] += learning_rate * grad * 5.0f;
                }
                else
                {
                    out_updated_model->weights[j] -= learning_rate * grad;
                }
            }
        }
    }
    out_updated_model->loss = total_loss / (float)pds->num_samples;
    return DIN_OK;
}

din_error_t din_pds_compute_model_delta(const din_model_t *initial_model, const din_model_t *updated_model, din_model_t *out_delta)
{
    if (!initial_model || !updated_model || !out_delta)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    din_model_copy(out_delta, updated_model);
    return din_model_sub(out_delta, initial_model);
}

void din_pds_set_byzantine(din_pds_t *pds, bool is_byzantine)
{
    if (pds)
    {
        pds->is_byzantine = is_byzantine;
    }
}
