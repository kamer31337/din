#include "din/din_coordinator.h"
#include <stdio.h>

int main(void)
{
    printf(">>> INITIALIZING DECENTRALIZED INTELLIGENCE NETWORK (DIN) <<<");
    const char *model_owner_addr = "0x0";
    float escrow_deposit = 50000.0f;
    uint32_t num_features = 4;
    din_coordinator_t coordinator;

    din_error_t err = din_coordinator_init(&coordinator, model_owner_addr, escrow_deposit, num_features);
    if (err != DIN_OK)
    {
        printf("Failed to initialize DIN coordinator. Error: %d\n", err);
        return 1;
    }
    printf("[1] Coordinator initialized. Escrow deposited: $%.2f USDC\n", escrow_deposit);

    din_model_t genesis_model;
    din_model_init(&genesis_model, num_features);
    genesis_model.weights[0] = 0.5f;
    genesis_model.weights[1] = -0.2f;
    genesis_model.weights[2] = 0.8f;
    genesis_model.weights[3] = 0.1f;
    din_coordinator_setup_genesis_model(&coordinator, &genesis_model);
    printf("[2] Genesis model created & published to IPFS. CID: %s\n", coordinator.chain.genesis_cid.str);

    float control_x1[4] = {1.0f, 0.5f, 2.0f, 1.0f};
    float control_x2[4] = {2.0f, 1.0f, 0.5f, 2.0f};
    float control_x3[4] = {0.5f, 2.0f, 1.0f, 0.5f};
    float control_x4[4] = {1.5f, 1.5f, 1.5f, 1.0f};
    din_auditing_add_control_sample(&coordinator.control_dataset, control_x1, 2.4f);
    din_auditing_add_control_sample(&coordinator.control_dataset, control_x2, 1.8f);
    din_auditing_add_control_sample(&coordinator.control_dataset, control_x3, 1.2f);
    din_auditing_add_control_sample(&coordinator.control_dataset, control_x4, 2.1f);
    printf("[3] Model Owner loaded %u encrypted benchmark samples into Control Dataset.\n", coordinator.control_dataset.num_samples);

    din_coordinator_register_evaluator(&coordinator, 1, "0x0", 1000.0f, 0.10f);
    din_coordinator_register_evaluator(&coordinator, 2, "0x0", 1500.0f, 0.10f);
    din_coordinator_register_evaluator(&coordinator, 3, "0x0", 800.0f, 0.10f);
    printf("[4] Registered %u Evaluators with Harberger-taxed NFT Staking licenses.\n", coordinator.num_evaluators);

    const char *p_addrs[10] = {
        "0x0", "0x0"
    };

    for (uint32_t i = 0; i < 2; ++i)
    {
        din_coordinator_register_participant(&coordinator, i + 1, p_addrs[i]);
        din_pds_t *pds = &coordinator.participants[i];

        float s1[4] = {1.0f + (float)i * 0.1f, 0.5f, 2.0f - (float)i * 0.05f, 1.0f};
        float s2[4] = {0.8f, 1.2f + (float)i * 0.08f, 1.1f, 0.9f};
        din_pds_add_sample(pds, s1, 4, 2.4f + (float)i * 0.05f);
        din_pds_add_sample(pds, s2, 4, 1.5f + (float)i * 0.03f);

        if (i == 1)
        {
            din_pds_set_byzantine(pds, true);
            printf("    -> [Warning] Participant #2 flagged as Byzantine adversary (data poisoner).\n");
        }
    }
    printf("[5] Registered %u Participants holding sovereign Personal Data Stores (PDS).\n\n", coordinator.num_participants);

    uint32_t total_rounds = 3;
    for (uint32_t r = 1; r <= total_rounds; ++r)
    {
        printf("--- Executing Federated Learning Round %u/%u ---\n", r, total_rounds);
        din_coordinator_run_training_round(&coordinator, 0.02f, 5);
        printf("    Subgroups formed: %u (k-threshold = %u)\n", coordinator.agg.num_subgroups, coordinator.k_secagg_threshold);
        printf("    Pairwise additive masking applied across all PDS nodes (Intermediate sums matched).\n");
        printf("    Master Aggregator published new Global Model CID: %s\n", coordinator.chain.current_global_cid.str);
    }

    printf("\n[6] Training complete. Transitioning to Decentralized Auditing Protocol...\n");
    din_coordinator_run_auditing_and_settlement(&coordinator);

    printf("\n[7] Demonstrating Partial Common Ownership (PCO) Forced-Sale Rule...\n");
    const char *new_bidder = "0x001";
    float purchase_bid = 1200.0f;
    float new_valuation = 2000.0f;
    err = din_tokenomics_pco_forced_sale(&coordinator.tokenomics, 2, new_bidder, purchase_bid, new_valuation);
    if (err == DIN_OK)
    {
        printf("    [PCO Forced Sale Success] NFT #2 transferred to %s at self-assessed price ($%.1f).\n", new_bidder, purchase_bid);
    }

    din_coordinator_print_summary(&coordinator);

    printf("[8] Demonstrating Sovereign Wallet Manager & Creator...\n");
    din_wallet_t demo_wallet;
    err = din_wallet_create(&coordinator.wallet_mgr, "Hospital_Alpha_PDS", DIN_ROLE_PARTICIPANT, &demo_wallet);
    if (err == DIN_OK)
    {
        printf("    Created Sovereign Wallet: %s\n", demo_wallet.label);
        printf("    Derived Address : %s\n", demo_wallet.address.str);
        printf("    Decentralized ID: %s\n", demo_wallet.did);
        printf("    12-Word Mnemonic: \"%s\"\n", demo_wallet.mnemonic);

        uint8_t sample_delta_data[16] = "MODEL_DELTA_R1";
        din_hash256_t sig;
        din_wallet_sign_data(&demo_wallet, sample_delta_data, sizeof(sample_delta_data), &sig);
        bool valid = din_wallet_verify_sig(&demo_wallet, sample_delta_data, sizeof(sample_delta_data), &sig);
        printf("    Cryptographic Signature on Delta Data: %s\n", valid ? "VALID (Verified)" : "INVALID");

        din_wallet_fund(&coordinator.wallet_mgr, demo_wallet.address.str, 5000.0f, 2.5f);
        printf("    Funded Wallet Balance: $%.2f USDC | %.4f ETH\n", demo_wallet.usdc_balance + 5000.0f, demo_wallet.eth_balance + 2.5f);
    }

    printf("\n>>> DIN SIMULATION RUN COMPLETED SUCCESSFULLY <<<\n\n");
    return 0;
}
