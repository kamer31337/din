#ifndef DIN_COORDINATOR_H
#define DIN_COORDINATOR_H

#include "din_types.h"
#include "din_crypto.h"
#include "din_model.h"
#include "din_pds.h"
#include "din_ipfs.h"
#include "din_blockchain.h"
#include "din_secagg.h"
#include "din_auditing.h"
#include "din_tokenomics.h"
#include "din_wallet.h"

typedef struct
{
    din_blockchain_t chain;
    din_ipfs_t ipfs;
    din_hierarchical_aggregator_t agg;
    din_tokenomics_engine_t tokenomics;
    din_wallet_manager_t wallet_mgr;
    din_control_dataset_t control_dataset;
    din_pds_t participants[DIN_MAX_PARTICIPANTS];
    uint32_t num_participants;
    uint32_t num_evaluators;
    uint32_t k_secagg_threshold;
    float outlier_threshold;
    din_model_t current_model;
} din_coordinator_t;

din_error_t din_coordinator_init(din_coordinator_t *coord, const char *model_owner_addr, float escrow_amount, uint32_t num_features);
din_error_t din_coordinator_register_participant(din_coordinator_t *coord, uint32_t p_id, const char *address);
din_error_t din_coordinator_register_evaluator(din_coordinator_t *coord, uint32_t e_id, const char *address, float valuation, float tax_rate);
din_error_t din_coordinator_setup_genesis_model(din_coordinator_t *coord, const din_model_t *initial_model);
din_error_t din_coordinator_run_training_round(din_coordinator_t *coord, float learning_rate, uint32_t local_epochs);
din_error_t din_coordinator_run_auditing_and_settlement(din_coordinator_t *coord);
void din_coordinator_print_summary(const din_coordinator_t *coord);

#endif
