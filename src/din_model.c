#include "din/din_model.h"
#include <math.h>
#include <string.h>

din_error_t din_model_init(din_model_t *model, uint32_t num_params)
{
    if (!model || num_params == 0 || num_params > DIN_MAX_PARAMS)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    model->num_params = num_params;
    model->round = 0;
    model->loss = 0.0f;
    for (uint32_t i = 0; i < num_params; ++i)
    {
        model->weights[i] = 0.0f;
    }
    return DIN_OK;
}

din_error_t din_model_copy(din_model_t *dest, const din_model_t *src)
{
    if (!dest || !src)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    dest->num_params = src->num_params;
    dest->round = src->round;
    dest->loss = src->loss;
    memcpy(dest->weights, src->weights, sizeof(float) * src->num_params);
    return DIN_OK;
}

din_error_t din_model_add(din_model_t *dest, const din_model_t *src)
{
    if (!dest || !src || dest->num_params != src->num_params)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    for (uint32_t i = 0; i < dest->num_params; ++i)
    {
        dest->weights[i] += src->weights[i];
    }
    return DIN_OK;
}

din_error_t din_model_sub(din_model_t *dest, const din_model_t *src)
{
    if (!dest || !src || dest->num_params != src->num_params)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    for (uint32_t i = 0; i < dest->num_params; ++i)
    {
        dest->weights[i] -= src->weights[i];
    }
    return DIN_OK;
}

din_error_t din_model_scale(din_model_t *model, float factor)
{
    if (!model)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    for (uint32_t i = 0; i < model->num_params; ++i)
    {
        model->weights[i] *= factor;
    }
    return DIN_OK;
}

din_error_t din_model_zero(din_model_t *model)
{
    if (!model)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    memset(model->weights, 0, sizeof(float) * model->num_params);
    model->loss = 0.0f;
    return DIN_OK;
}

din_error_t din_model_apply_delta(din_model_t *base, const din_model_t *delta)
{
    if (!base || !delta || base->num_params != delta->num_params)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    for (uint32_t i = 0; i < base->num_params; ++i)
    {
        base->weights[i] += delta->weights[i];
    }
    return DIN_OK;
}

float din_model_compute_l2_norm(const din_model_t *model)
{
    if (!model)
    {
        return 0.0f;
    }
    float sum_sq = 0.0f;
    for (uint32_t i = 0; i < model->num_params; ++i)
    {
        sum_sq += model->weights[i] * model->weights[i];
    }
    return sqrtf(sum_sq);
}

size_t din_model_serialize(const din_model_t *model, uint8_t *out_buffer, size_t max_len)
{
    size_t required = sizeof(uint32_t) * 2 + sizeof(float) + sizeof(float) * model->num_params;
    if (!model || !out_buffer || max_len < required)
    {
        return 0;
    }
    uint8_t *ptr = out_buffer;
    memcpy(ptr, &model->num_params, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    memcpy(ptr, &model->round, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    memcpy(ptr, &model->loss, sizeof(float));
    ptr += sizeof(float);
    memcpy(ptr, model->weights, sizeof(float) * model->num_params);
    return required;
}

din_error_t din_model_deserialize(din_model_t *model, const uint8_t *in_buffer, size_t len)
{
    if (!model || !in_buffer || len < sizeof(uint32_t) * 2 + sizeof(float))
    {
        return DIN_ERR_INVALID_PARAM;
    }
    const uint8_t *ptr = in_buffer;
    memcpy(&model->num_params, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    if (model->num_params > DIN_MAX_PARAMS)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    memcpy(&model->round, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    memcpy(&model->loss, ptr, sizeof(float));
    ptr += sizeof(float);
    size_t weight_bytes = sizeof(float) * model->num_params;
    if (len < sizeof(uint32_t) * 2 + sizeof(float) + weight_bytes)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    memcpy(model->weights, ptr, weight_bytes);
    return DIN_OK;
}

float din_model_predict_linear(const din_model_t *model, const float *features, uint32_t feature_dim)
{
    if (!model || !features)
    {
        return 0.0f;
    }
    float pred = 0.0f;
    uint32_t limit = (model->num_params < feature_dim) ? model->num_params : feature_dim;
    for (uint32_t i = 0; i < limit; ++i)
    {
        pred += model->weights[i] * features[i];
    }
    return pred;
}
