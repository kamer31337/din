#ifndef DIN_PDS_H
#define DIN_PDS_H

#include "din_types.h"
#include "din_model.h"

typedef struct
{
    uint32_t participant_id;
    din_addr_t address;
    uint32_t num_samples;
    uint32_t feature_dim;
    float features[DIN_MAX_DATASET_SIZE][DIN_MAX_PARAMS];
    float targets[DIN_MAX_DATASET_SIZE];
    bool is_byzantine;
} din_pds_t;

din_error_t din_pds_init(din_pds_t *pds, uint32_t participant_id, const char *address);
din_error_t din_pds_add_sample(din_pds_t *pds, const float *features, uint32_t feature_dim, float target);
din_error_t din_pds_train_local_step(din_pds_t *pds, const din_model_t *global_model, din_model_t *out_updated_model, float learning_rate, uint32_t epochs);
din_error_t din_pds_compute_model_delta(const din_model_t *initial_model, const din_model_t *updated_model, din_model_t *out_delta);
void din_pds_set_byzantine(din_pds_t *pds, bool is_byzantine);

#endif
