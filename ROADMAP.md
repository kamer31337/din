# Decentralized Intelligence Network (DIN) Roadmap

This document outlines the multi-phase engineering and research roadmap for the **Decentralized Intelligence Network (DIN)** C implementation, based on the framework introduced in arXiv:2407.02461v5.

---

## Strategic Phases & Milestones

```
  Phase 1: Foundations & Sovereign Data  [COMPLETED]
     ├── Cryptographic hashing (SHA-256 / Blake2b)
     ├── Content Identifier (CID) generation (IPFS spec)
     ├── Model parameters, tensor operations, & serialization
     └── Sovereign Personal Data Store (PDS) boundary
  
  Phase 2: Storage & Blockchain Coordination  [COMPLETED]
     ├── Off-chain Content-Addressed Storage (CAS / IPFS)
     ├── Smart Contract state machine & lifecycle events
     ├── Escrow deposit & prize vault accounting
     └── Round delineation & participant enrollment
  
  Phase 3: Hierarchical Secure Aggregation  [COMPLETED]
     ├── Bonawitz et al. multi-tier aggregation topology
     ├── Pairwise symmetric additive blinding mask generation
     ├── Subgroup intermediate sum aggregation (k-anonymity)
     └── Master Aggregator global model update synthesis
  
  Phase 4: Decentralized Auditing Protocol  [COMPLETED]
     ├── Role delineation: Participants vs Evaluators (Q << N)
     ├── Encrypted control dataset distribution & isolation
     ├── Commit-then-reveal protocol for evaluation scores
     └── Median scoring & Byzantine outlier rejection (|s - M| > 0.5)
  
  Phase 5: Tokenomics, Harberger Tax & PCO  [COMPLETED]
     ├── NFT Staking registry for Evaluator licenses
     ├── Self-assessed valuation & Harberger tax calculation
     ├── Partial Common Ownership (PCO) forced-sale mechanics
     └── Dynamic reward distribution (97% participants / 3% evaluators)

  Phase 6: Sovereign Wallet Creator & Manager  [COMPLETED]
     ├── 12-Word BIP-39 mnemonic seed phrase generation
     ├── Deterministic keypair and Ethereum-compatible 0x address derivation
     ├── Decentralized Identifiers (DID: did:din:0x...)
     ├── Cryptographic delta payload signing and verification
     └── Multi-wallet registry, USDC balances, and keystore persistence

  Phase 7: Native Win32 / WinApi Desktop GUI Console  [COMPLETED]
     ├── Pure WinApi GDI double-buffered dark dashboard
     ├── Live metric cards (Escrow, Treasury, Round, CID)
     ├── Interactive step-by-step FL lifecycle toolbar
     ├── Multi-column ListView tables for PDS & Staked Evaluators
     └── Real-time auto-scrolling cryptographic log stream
  
  Phase 8: Hardware TEE & Production Extensions  [PLANNED]
     ├── Hardware Trusted Execution Environments (Intel SGX / AMD SEV)
     ├── Zero-Knowledge Proof (zk-SNARK / Groth16) verification
     ├── Native libp2p C daemon networking
     └── Gov4Git quadratic voting & DPP proposal protocol
```

---

## Detailed Milestone Breakdown

### Phase 1: Foundations & Sovereign Data Boundary
- [x] Standard C99/C11 portable types and error handling (`din_types.h`).
- [x] Cryptographic utilities: SHA-256 hashing, CID string formatting (`din_crypto.h`, `din_crypto.c`).
- [x] Vector and matrix math for neural network weights: local gradient steps, FedAvg computations, parameter serialization (`din_model.h`, `din_model.c`).
- [x] Personal Data Store (PDS) abstraction: strict encapsulation of local sovereign datasets, guaranteed zero raw data leakage (`din_pds.h`, `din_pds.c`).

### Phase 2: Decentralized Storage & Blockchain Coordination
- [x] IPFS Content-Addressed Storage simulation: storing model checkpoints and encrypted deltas by SHA-256 CID (`din_ipfs.h`, `din_ipfs.c`).
- [x] On-chain Smart Contract simulation: registration of model owners, genesis model CID, training rounds, and reward escrow deposit (`din_blockchain.h`, `din_blockchain.c`).
- [x] Phase transitions: `DIN_PHASE_GENESIS`, `DIN_PHASE_TRAINING`, `DIN_PHASE_AGGREGATION`, `DIN_PHASE_AUDITING`, `DIN_PHASE_REWARD_SETTLED`.

### Phase 3: Hierarchical Secure Aggregation
- [x] Subgroup aggregation topology ($k$-device threshold) to scale beyond quadratic communication boundaries (`din_secagg.h`, `din_secagg.c`).
- [x] Symmetric pairwise masking:
  $$\sum_{i \in \mathcal{G}} y_i = \sum_{i \in \mathcal{G}} x_i$$
- [x] Subgroup aggregators forward intermediate masked sums to Master Aggregator.
- [x] Master Aggregator produces global weight updates and registers new CID on-chain.

### Phase 4: Decentralized Auditing & Byzantine Fault Tolerance
- [x] Dynamic evaluator selection: ratio $Q \ll N$ (e.g. 1 evaluator per 10 participants) (`din_auditing.h`, `din_auditing.c`).
- [x] Control dataset encrypted by Model Owner and made available for isolated benchmarking.
- [x] Two-phase commit-reveal for evaluation scores with cryptographic hash commitments.
- [x] Median scoring filter:
  $$|s_{i,q} - \operatorname{median}(S_i)| > 0.5 \implies s_{i,q} = 0$$
- [x] Slashing of dishonest or fabricating evaluators.

### Phase 5: Tokenomics, Harberger Tax & Partial Common Ownership (PCO)
- [x] Evaluator NFT license staking (`din_tokenomics.h`, `din_tokenomics.c`).
- [x] Self-assessed valuation $V$ and continuous Harberger tax calculation $\tau \cdot V$.
- [x] Forced-sale mechanism: automatic transfer upon receipt of bid $P \ge V$.
- [x] Escrow payout execution:
  - 97% to honest data-contributing participants.
  - 3% to verified evaluators.
  - Harberger taxes routed to the Public Goods Treasury (supporting DIN development & Gitcoin grants).

### Phase 6: Native Win32 / WinApi Desktop GUI Console
- [x] Modern native Win32 window architecture (`din_gui.h`, `din_gui.c`, `main_gui.c`).
- [x] Custom double-buffered GDI rendering (`BitBlt`) with a sleek dark slate theme.
- [x] Interactive step-by-step FL round triggers and automated full-flow execution.
- [x] Live PDS Participant and Staked Evaluator ListViews with dynamic columns.
- [x] Real-time event log console stream.

### Phase 7: Hardware TEE & Production Extensions (Future Scope)
- [ ] Direct hardware enclave support (Intel SGX SDK / AMD SEV) for control dataset evaluation.
- [ ] On-chain zk-SNARK / ZKP score verification module.
- [ ] Live libp2p network transport replacing mock socket communication.
- [ ] Integration with Ethereum / EVM testnets via JSON-RPC.
- [ ] Implementation of DPP (DIN Proposal Protocols) with Gov4Git quadratic voting.

---

## Verification & Quality Assurance

Each phase is validated by:
1. Unit tests for mathematical invariance (mask cancellation, median calculation, Harberger pricing).
2. End-to-end CLI simulation harness (`examples/main_simulation.c`) simulating nodes, Byzantine actors, and complete escrow distribution.
3. Native WinApi Desktop GUI application (`examples/main_gui.c`).
4. Strict verification of user clang formatting rules (single-line prototypes/definitions, brace placement, single-line variable declarations).
