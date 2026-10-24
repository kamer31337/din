#ifndef DIN_MODEL_H
#define DIN_MODEL_H

#include "din_types.h"

typedef struct
{
    uint32_t num_params;
    float weights[DIN_MAX_PARAMS];
    uint32_t round;
    float loss;
} din_model_t;

din_error_t din_model_init(din_model_t *model, uint32_t num_params);
din_error_t din_model_copy(din_model_t *dest, const din_model_t *src);
din_error_t din_model_add(din_model_t *dest, const din_model_t *src);
din_error_t din_model_sub(din_model_t *dest, const din_model_t *src);
din_error_t din_model_scale(din_model_t *model, float factor);
din_error_t din_model_zero(din_model_t *model);
din_error_t din_model_apply_delta(din_model_t *base, const din_model_t *delta);
float din_model_compute_l2_norm(const din_model_t *model);
size_t din_model_serialize(const din_model_t *model, uint8_t *out_buffer, size_t max_len);
din_error_t din_model_deserialize(din_model_t *model, const uint8_t *in_buffer, size_t len);
float din_model_predict_linear(const din_model_t *model, const float *features, uint32_t feature_dim);

#endif
