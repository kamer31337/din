# Decentralized Intelligence Network (DIN) in C

A production-grade, modular C implementation of the **Decentralized Intelligence Network (DIN)** architecture, designed according to the research paper:
> **Decentralized Intelligence Network (DIN)**  
> *Abraham Nash, University of Oxford* (arXiv:2407.02461v5)

---

## 1. Overview & Theoretical Framework

Modern Artificial Intelligence (AI) faces a fundamental paradox: high-performance models require immense amounts of varied data, yet high-value data is locked within isolated silos or individual personal environments due to privacy concerns and incentive misalignment. Centralized platforms create monopolistic walled gardens where data creators surrender data sovereignty without fair compensation.

**Decentralized Intelligence Network (DIN)** resolves this conflict by unifying:
1. **Personal Data Stores (PDS):** Sovereign data repositories under the exclusive management of participants. Raw data is strictly isolated and never leaves the edge device.
2. **Blockchain-Coordinated Federated Learning (FL):** Trustless protocol management and state delineation orchestrated via public smart contracts, with model updates content-addressed on the InterPlanetary File System (IPFS) to circumvent on-chain storage bottlenecks.
3. **Hierarchical Secure Aggregation:** A multi-layered aggregation topology (adapting Bonawitz et al., 2019) where Master Aggregators delegate to subgroup aggregators ($k$-anonymity threshold) to eliminate the quadratic communication overhead of pairwise masking.
4. **Decentralized Auditing Protocol:** Delineation of network roles into **Participants** (data providers and local trainers) and **Evaluators** (staked auditors, ratio $Q \ll N$, e.g., 1:10). Evaluators benchmark participant model contributions against an encrypted control dataset provided by the Model Owner.
5. **Robust Contributivity Scoring & Outlier Rejection:** Implementation of median scoring (inspired by BlockFlow and 2CP) where any evaluation diverging by more than 0.5 from the median is rejected ($|s_i - \text{median}(S)| > 0.5 \implies s_i = 0$), neutralizing Byzantine collusion and false reporting.
6. **Tokenomics, Harberger Taxation, and Partial Common Ownership (PCO):** Evaluators stake Non-Fungible Tokens (NFTs) representing evaluation licenses. Evaluators self-assess their stake valuation $V$, pay continuous Harberger tax $\tau \cdot V$ into a public goods treasury, and must accept forced sales at their self-assessed price, aligning economic incentives and preventing monopolistic squatting.

---

## 2. Protocol Workflow

The complete training and auditing lifecycle executes in the following phases:

```
+---------------------------------------------------------------------------------------------------+
|                                      DIN PROTOCOL LIFECYCLE                                       |
+---------------------------------------------------------------------------------------------------+
  [Model Owner]                                                                                    
        |                                                                                          
        | 1. Deploy Smart Contracts & Escrow Reward Pool                                            
        | 2. Upload Genesis Model W_0 to IPFS (Get CID_0)                                          
        v                                                                                          
  [Smart Contract State Machine] <------------------------------------+                            
        |                                                             |                            
        | 3. Publish Round t Announcement (CID_t)                     |                            
        v                                                             |                            
  [Participants (PDS)]                                                |                            
        |                                                             |                            
        | 4. Fetch W_t from IPFS                                      |                            
        | 5. Run Local Training on Private Sovereign Data             |                            
        | 6. Apply Pairwise Additive Blinding Masks                   | (Next Round Iteration)     
        | 7. Upload Encrypted Delta to IPFS (CID_i,t)                 |                            
        | 8. Record CID_i,t on Coordinator Smart Contract             |                            
        v                                                             |                            
  [Hierarchical Aggregation]                                          |                            
        |                                                             |                            
        | 9. Subgroup Aggregators fetch updates for group G_m (>= k)  |                            
        | 10. Intermediate Aggregation (Mask cancellation)            |                            
        | 11. Master Aggregator merges subgroup models into W_t+1      |                            
        | 12. Register W_t+1 CID on Smart Contract                    |                            
        v                                                             |                            
  [Convergence Check]                                                 |                            
        |                                                             |                            
        +---> If not converged: Repeat Round -------------------------+                            
        |                                                                                          
        +---> If satisfied / Final round signaled: Proceed to Auditing                             
                    v                                                                              
              [Decentralized Auditing]                                                             
                    |                                                                              
                    | 13. Model Owner encrypts Control Dataset D_ctrl                              
                    | 14. Smart Contract selects Evaluator subset (Q << N)                         
                    | 15. Evaluators benchmark Participant models on D_ctrl                        
                    | 16. Evaluators commit hashed scores + ZKP proofs                             
                    | 17. Evaluators reveal scores; Contract applies Median Outlier Filter         
                    v                                                                              
              [Reward Settlement & Harberger Taxation]                                             
                    |                                                                              
                    | 18. Compute Contributivity Fractions (Substra / Shapley / Median)            
                    | 19. Distribute Escrow: 97% to Participants, 3% to Evaluators                
                    | 20. Collect Harberger Tax from Evaluator NFTs into Public Goods Pool         
                    v                                                                              
              [Final Model Release]                                                                
```

---

## 3. Mathematical Foundations

### 3.1 Pairwise Additive Blinding for Secure Aggregation
For each subgroup $\mathcal{G}$ with $|\mathcal{G}| \ge k$, participants $i, j \in \mathcal{G}$ generate symmetric pairwise masks:
$$s_{i,j} = -s_{j,i} \quad (\forall i < j)$$
Each participant masks their model update $x_i$:
$$y_i = x_i + \sum_{j \in \mathcal{G}, j > i} s_{i,j} - \sum_{j \in \mathcal{G}, j < i} s_{j,i}$$
When aggregated by the Subgroup Aggregator:
\sum_{i \in \mathcal{G}} y_i = \sum_{i \in \mathcal{G}} x_i + \sum_{i \in \mathcal{G}} \sum_{j \in \mathcal{G}, j \ne i} \operatorname{sgn}(j - i) s_{\min(i,j), \max(i,j)} = \sum_{i \in \mathcal{G}} x_i
All masks cancel out cleanly while individual vectors remain strictly obfuscated to the aggregator.

### 3.2 Median Scoring & Byzantine Slashing
For model $i$, evaluators submit scores $S_i = \{s_{i,1}, s_{i,2}, \dots, s_{i,Q}\}$.
Let $M_i = \operatorname{median}(S_i)$.
The validated score assigned to evaluator $q$ for assessing participant $i$ is penalized if it deviates beyond a strict boundary:
$$\tilde{s}_{i,q} = \begin{cases} s_{i,q} & \text{if } |s_{i,q} - M_i| \le 0.5 \\ 0 & \text{if } |s_{i,q} - M_i| > 0.5 \end{cases}$$
The final aggregate participant score is normalized across all honest evaluations:
$$\bar{S}_i = \frac{1}{|\{q : |s_{i,q} - M_i| \le 0.5\}|} \sum_{q : |s_{i,q} - M_i| \le 0.5} s_{i,q}$$

### 3.3 Harberger Taxation & Partial Common Ownership (PCO)
Evaluator NFT license holding an asset valuation $V_e$ is taxed at rate $\tau$ over epoch $\Delta t$:
$$\text{Tax}_e = V_e \cdot \tau \cdot \Delta t$$
- **Property 1 (Self-Assessment):** The evaluator freely chooses $V_e$.
- **Property 2 (Forced Sale):** Any third-party bidder providing $P \ge V_e$ immediately purchases the NFT evaluation license at price $V_e$.
- **Equilibrium:** Under-assessing $V_e$ risks losing the license cheaply to competitors; over-assessing $V_e$ incurs heavy tax burdens into the Public Goods treasury.

---

## 4. Native Windows GUI (WinApi)

In addition to the headless command-line simulation, the project features a native **Win32 / WinApi Desktop Application** (`din_gui.exe`):

### GUI Features:
- **Dark Theme Dashboard:** Double-buffered GDI rendering (`WM_PAINT` / `BitBlt`) with sleek dark theme palette (RGB 24, 24, 37 background, RGB 30, 30, 46 cards).
- **Live Metric Cards:** Real-time visualization of Escrow Vault pool ($ USDC), Public Goods Treasury balance, active FL Training Round, and IPFS Global Model CID.
- **Interactive Control Toolbar:**
  - `1. Init & Escrow`: Deploys contracts, deposits escrow, creates genesis model.
  - `2. Enroll Nodes`: Registers sovereign PDS participants and staked evaluator NFTs.
  - `3. Run FL Round`: Triggers local PDS training, pairwise masking, and hierarchical subgroup aggregation.
  - `4. Run Audit`: Executes encrypted control benchmarking, commit-reveal verification, and median outlier slashing.
  - `5. Settle & Tax`: Applies Harberger taxation, triggers PCO forced sales, and releases 97%/3% rewards.
  - `Run Full Flow`: Runs complete automated lifecycle.
  - `Reset System`: Clears state for fresh experimentation.
- **Sovereign PDS Grid:** Displays participants, addresses, data isolation status, Byzantine detection flags, model CIDs, and payout allocations.
- **Evaluator NFT Staking Registry:** Displays auditors, self-assessed valuations, Harberger taxes paid to public goods, reward earnings, and slashing status.
- **Real-Time Event Console:** Live multiline stream of cryptographic events, IPFS transactions, and state changes.

---

## 5. Directory Structure

```
d:/DIN/
├── CMakeLists.txt                # CMake build script (CLI & WinApi GUI)
├── Makefile                      # Standard Makefile for GCC/Clang/MSVC
├── README.md                     # Comprehensive project documentation
├── ROADMAP.md                    # Multi-phase project roadmap
├── include/
│   └── din/
│       ├── din_types.h           # System primitives, limits, and role types
│       ├── din_crypto.h          # Cryptographic hashing, CID, masking
│       ├── din_model.h           # Tensor structures, serialization, FedAvg
│       ├── din_pds.h             # Sovereign Personal Data Store engine
│       ├── din_ipfs.h            # Content-addressed storage abstraction
│       ├── din_blockchain.h     # Public blockchain & Smart Contract state
│       ├── din_secagg.h          # Hierarchical Secure Aggregation
│       ├── din_auditing.h        # Auditing, evaluation & median scoring
│       ├── din_tokenomics.h      # Harberger tax, PCO auction, rewards
│       ├── din_wallet.h          # Sovereign Wallet Creator & Manager
│       ├── din_wallet_gui.h      # Native Win32 Wallet UI interface
│       ├── din_coordinator.h     # High-level round orchestrator
│       └── din_gui.h             # Native Win32 / WinApi GUI interface
├── src/
│   ├── din_crypto.c
│   ├── din_model.c
│   ├── din_pds.c
│   ├── din_ipfs.c
│   ├── din_blockchain.c
│   ├── din_secagg.c
│   ├── din_auditing.c
│   ├── din_tokenomics.c
│   ├── din_wallet.c              # Wallet generation, mnemonic, DID
│   ├── din_wallet_gui.c          # Sovereign Wallet UI window implementation
│   ├── din_coordinator.c
│   └── din_gui.c                 # Native Win32 GUI implementation
└── examples/
    ├── main_simulation.c        # Command-line simulation runner
    └── main_gui.c               # Native Windows GUI application entrypoint
```

---

## 6. Coding Style & Clang Strict Guidelines

This codebase strictly adheres to the mandated C formatting requirements:
1. **Single-Line Function Signatures:**
   All function declarations and definitions start and finish on a single line. The opening brace `{` is placed on the following line:
   ```c
   din_error_t din_model_create(din_model_t *model, uint32_t num_params)
   {
       /* function body */
   }
   ```
2. **Compact Variable Declarations:**
   Variables are declared and initialized on single lines without split breaks:
   ```c
   float learning_rate = 0.01f;
   uint32_t num_epochs = 5;
   ```
3. **No Unnecessary Line Breaks:**
   Loops, arrays, and branch conditions maintain concise, uncluttered syntax.
4. **Pure C Standard:**
   Clean C99/C11 portable implementation without external library baggage.

---

## 7. Build & Execution Instructions

### Build with Makefile:
```bash
# Build both CLI simulation and WinApi GUI
make

# Run CLI simulation
make run
# or: ./build/din_simulation

# Run Native Win32 GUI
make run-gui
# or: ./build/din_gui.exe
```

### Build with CMake:
```bash
mkdir -p build && cd build
cmake ..
cmake --build .

# Run CLI
./din_simulation

# Run WinApi GUI
./din_gui.exe
```

---

## 8. Citation

```bibtex
@article{nash2024din,
  title   = {Decentralized Intelligence Network (DIN)},
  author  = {Nash, Abraham},
  journal = {arXiv preprint arXiv:2407.02461v5},
  year    = {2024},
  url     = {https://arxiv.org/abs/2407.02461}
}
```
