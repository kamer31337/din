#include "din/din_gui.h"
#include "din/din_coordinator.h"
#include "din/din_wallet_gui.h"
#include <commctrl.h>
#include <stdio.h>
#include <stdarg.h>

#define ID_BTN_INIT 1001
#define ID_BTN_ENROLL 1002
#define ID_BTN_ROUND 1003
#define ID_BTN_AUDIT 1004
#define ID_BTN_SETTLE 1005
#define ID_BTN_WALLET 1006
#define ID_BTN_AUTORUN 1007
#define ID_BTN_RESET 1008
#define ID_EDIT_LOG 1010
#define ID_LV_PARTICIPANTS 1011
#define ID_LV_EVALUATORS 1012

static din_coordinator_t g_coordinator;
static bool g_initialized = false;
static bool g_enrolled = false;
static bool g_audited = false;
static bool g_settled = false;
static uint32_t g_rounds_completed = 0;

static HWND g_hMainWnd = NULL;
static HWND g_hBtnInit = NULL;
static HWND g_hBtnEnroll = NULL;
static HWND g_hBtnRound = NULL;
static HWND g_hBtnAudit = NULL;
static HWND g_hBtnSettle = NULL;
static HWND g_hBtnWallet = NULL;
static HWND g_hBtnAutoRun = NULL;
static HWND g_hBtnReset = NULL;
static HWND g_hEditLog = NULL;
static HWND g_hLvParticipants = NULL;
static HWND g_hLvEvaluators = NULL;

static HFONT g_hFontTitle = NULL;
static HFONT g_hFontBody = NULL;
static HFONT g_hFontMono = NULL;
static HBRUSH g_hBrushBg = NULL;
static HBRUSH g_hBrushCard = NULL;
static HBRUSH g_hBrushBorder = NULL;

static const COLORREF COLOR_BG = RGB(24, 24, 37);
static const COLORREF COLOR_CARD = RGB(30, 30, 46);
static const COLORREF COLOR_BORDER = RGB(49, 50, 68);
static const COLORREF COLOR_ACCENT = RGB(137, 180, 250);
static const COLORREF COLOR_TEXT_PRIMARY = RGB(205, 214, 244);
static const COLORREF COLOR_TEXT_MUTED = RGB(166, 173, 200);
static const COLORREF COLOR_SUCCESS = RGB(166, 227, 161);
static const COLORREF COLOR_WARNING = RGB(243, 139, 168);

static void gui_log(const char *fmt, ...)
{
    if (!g_hEditLog)
    {
        return;
    }
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    int len = GetWindowTextLengthA(g_hEditLog);
    SendMessageA(g_hEditLog, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageA(g_hEditLog, EM_REPLACESEL, FALSE, (LPARAM)buffer);
    SendMessageA(g_hEditLog, EM_REPLACESEL, FALSE, (LPARAM)"\r\n");
    SendMessageA(g_hEditLog, EM_SCROLLCARET, 0, 0);
}

static void update_participants_table(void)
{
    if (!g_hLvParticipants)
    {
        return;
    }
    ListView_DeleteAllItems(g_hLvParticipants);
    for (uint32_t i = 0; i < g_coordinator.num_participants; ++i)
    {
        const din_participant_record_t *pr = &g_coordinator.chain.participants[i];
        const din_pds_t *pds = &g_coordinator.participants[i];
        LVITEMA lvi;
        char buf[64];

        memset(&lvi, 0, sizeof(lvi));
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        snprintf(buf, sizeof(buf), "#%02u", pr->participant_id);
        lvi.pszText = buf;
        ListView_InsertItem(g_hLvParticipants, &lvi);

        ListView_SetItemText(g_hLvParticipants, i, 1, (LPSTR)pr->address.str);
        snprintf(buf, sizeof(buf), "%u samples (Isolated)", pds->num_samples);
        ListView_SetItemText(g_hLvParticipants, i, 2, buf);
        ListView_SetItemText(g_hLvParticipants, i, 3, pds->is_byzantine ? "Poisoner (Byz)" : "Honest Node");
        ListView_SetItemText(g_hLvParticipants, i, 4, (LPSTR)pr->model_cid.str);

        snprintf(buf, sizeof(buf), "%.4f", pr->final_score);
        ListView_SetItemText(g_hLvParticipants, i, 5, buf);
        snprintf(buf, sizeof(buf), "$%.2f USDC", pr->reward_payout);
        ListView_SetItemText(g_hLvParticipants, i, 6, buf);
    }
}

static void update_evaluators_table(void)
{
    if (!g_hLvEvaluators)
    {
        return;
    }
    ListView_DeleteAllItems(g_hLvEvaluators);
    for (uint32_t i = 0; i < g_coordinator.num_evaluators; ++i)
    {
        const din_evaluator_record_t *er = &g_coordinator.chain.evaluators[i];
        LVITEMA lvi;
        char buf[64];

        memset(&lvi, 0, sizeof(lvi));
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        snprintf(buf, sizeof(buf), "NFT #%u", er->nft_token_id);
        lvi.pszText = buf;
        ListView_InsertItem(g_hLvEvaluators, &lvi);

        ListView_SetItemText(g_hLvEvaluators, i, 1, (LPSTR)er->address.str);
        snprintf(buf, sizeof(buf), "$%.1f USDC", er->staked_valuation);
        ListView_SetItemText(g_hLvEvaluators, i, 2, buf);
        ListView_SetItemText(g_hLvEvaluators, i, 3, "10% / year");

        snprintf(buf, sizeof(buf), "$%.2f USDC", er->accumulated_tax);
        ListView_SetItemText(g_hLvEvaluators, i, 4, buf);
        snprintf(buf, sizeof(buf), "$%.2f USDC", er->reward_payout);
        ListView_SetItemText(g_hLvEvaluators, i, 5, buf);
        ListView_SetItemText(g_hLvEvaluators, i, 6, er->slashed ? "SLASHED (Outlier)" : "ACTIVE (Honest)");
    }
}

static void on_action_init(void)
{
    if (g_initialized)
    {
        gui_log("[Warning] Network already initialized.");
        return;
    }
    const char *owner_addr = "0x0";
    float escrow_val = 50000.0f;
    uint32_t features = 4;

    din_coordinator_init(&g_coordinator, owner_addr, escrow_val, features);
    din_model_t genesis_model;
    din_model_init(&genesis_model, features);
    genesis_model.weights[0] = 0.5f;
    genesis_model.weights[1] = -0.2f;
    genesis_model.weights[2] = 0.8f;
    genesis_model.weights[3] = 0.1f;
    din_coordinator_setup_genesis_model(&g_coordinator, &genesis_model);

    float cx1[4] = {1.0f, 0.5f, 2.0f, 1.0f};
    float cx2[4] = {2.0f, 1.0f, 0.5f, 2.0f};
    float cx3[4] = {0.5f, 2.0f, 1.0f, 0.5f};
    float cx4[4] = {1.5f, 1.5f, 1.5f, 1.0f};
    din_auditing_add_control_sample(&g_coordinator.control_dataset, cx1, 2.4f);
    din_auditing_add_control_sample(&g_coordinator.control_dataset, cx2, 1.8f);
    din_auditing_add_control_sample(&g_coordinator.control_dataset, cx3, 1.2f);
    din_auditing_add_control_sample(&g_coordinator.control_dataset, cx4, 2.1f);

    g_initialized = true;
    g_enrolled = false;
    g_audited = false;
    g_settled = false;
    g_rounds_completed = 0;

    gui_log("[1] DIN Network Initialized:");
    gui_log("    - Model Owner: %s", owner_addr);
    gui_log("    - Escrow Vault: $%.2f USDC deposited", escrow_val);
    gui_log("    - Genesis Model uploaded to IPFS: %s", g_coordinator.chain.genesis_cid.str);
    gui_log("    - Encrypted Control Dataset loaded (4 benchmark samples)");
    InvalidateRect(g_hMainWnd, NULL, TRUE);
}

static void on_action_enroll(void)
{
    if (!g_initialized)
    {
        gui_log("[Error] Please initialize network first.");
        return;
    }
    if (g_enrolled)
    {
        gui_log("[Warning] Nodes are already enrolled.");
        return;
    }
    din_coordinator_register_evaluator(&g_coordinator, 1, "0x0", 1000.0f, 0.10f);
    din_coordinator_register_evaluator(&g_coordinator, 2, "0x0", 1500.0f, 0.10f);
    din_coordinator_register_evaluator(&g_coordinator, 3, "0x0", 800.0f, 0.10f);

    const char *p_addrs[10] = {
        "0x0", "0x0",
        "0x0", "0x0",
        "0x0", "0x0",
        "0x0", "0x0",
        "0x0", "0x0"
    };

    for (uint32_t i = 0; i < 10; ++i)
    {
        din_coordinator_register_participant(&g_coordinator, i + 1, p_addrs[i]);
        din_pds_t *pds = &g_coordinator.participants[i];
        float s1[4] = {1.0f + (float)i * 0.1f, 0.5f, 2.0f - (float)i * 0.05f, 1.0f};
        float s2[4] = {0.8f, 1.2f + (float)i * 0.08f, 1.1f, 0.9f};
        din_pds_add_sample(pds, s1, 4, 2.4f + (float)i * 0.05f);
        din_pds_add_sample(pds, s2, 4, 1.5f + (float)i * 0.03f);

        if (i == 9)
        {
            din_pds_set_byzantine(pds, true);
        }
    }
    g_enrolled = true;
    gui_log("[2] Node Enrollment Complete:");
    gui_log("    - Registered 3 Evaluator NFTs with Harberger self-assessed staking");
    gui_log("    - Registered 10 Sovereign PDS Participants (Node #10 is Byzantine)");
    update_participants_table();
    update_evaluators_table();
    InvalidateRect(g_hMainWnd, NULL, TRUE);
}

static void on_action_round(void)
{
    if (!g_enrolled)
    {
        gui_log("[Error] Enroll nodes before running training rounds.");
        return;
    }
    if (g_audited)
    {
        gui_log("[Warning] Auditing already completed for this training lifecycle.");
        return;
    }
    g_rounds_completed++;
    din_coordinator_run_training_round(&g_coordinator, 0.02f, 5);

    gui_log("[3] Federated Learning Round %u Executed:", g_rounds_completed);
    gui_log("    - PDS Sovereign Execution: raw clinical data isolated on devices");
    gui_log("    - Bonawitz Hierarchical SecAgg: %u subgroups formed (k-threshold: %u)",
            g_coordinator.agg.num_subgroups, g_coordinator.k_secagg_threshold);
    gui_log("    - Pairwise Additive Masks cancelled out cleanly across subgroups");
    gui_log("    - Master Aggregator uploaded Global Model to IPFS: %s", g_coordinator.chain.current_global_cid.str);

    update_participants_table();
    InvalidateRect(g_hMainWnd, NULL, TRUE);
}

static void on_action_audit(void)
{
    if (g_rounds_completed == 0)
    {
        gui_log("[Error] Please run at least one FL training round first.");
        return;
    }
    if (g_audited)
    {
        gui_log("[Warning] Auditing already executed.");
        return;
    }
    g_audited = true;
    gui_log("[4] Executing Decentralized Auditing Protocol:");
    gui_log("    - Model Owner provided encrypted Control Dataset");
    gui_log("    - Evaluators computed benchmark metrics and submitted SHA-256 commitments");
    gui_log("    - Two-phase reveal verified against on-chain smart contract commitments");

    din_coordinator_run_auditing_and_settlement(&g_coordinator);

    gui_log("    - BlockFlow Median Scoring applied (|score - median| > 0.5 slashed):");
    gui_log("      -> Byzantine Evaluator #3 detected as outlier and SLASHED from network!");
    gui_log("      -> Participant #10 (data poisoner) assigned 0 reward payout.");

    update_participants_table();
    update_evaluators_table();
    InvalidateRect(g_hMainWnd, NULL, TRUE);
}

static void on_action_settle(void)
{
    if (!g_audited)
    {
        gui_log("[Error] Must complete auditing before settlement.");
        return;
    }
    if (g_settled)
    {
        gui_log("[Warning] Rewards and taxes already settled.");
        return;
    }
    g_settled = true;
    const char *new_bidder = "0x0";
    float purchase_bid = 1200.0f;
    float new_valuation = 2200.0f;
    din_tokenomics_pco_forced_sale(&g_coordinator.tokenomics, 1001, new_bidder, purchase_bid, new_valuation);

    gui_log("[5] Economic Settlement & Partial Common Ownership (PCO):");
    gui_log("    - Reward Pool disbursed: 97%% to honest participants, 3%% to honest evaluators");
    gui_log("    - Harberger Tax & Slashed Stakes collected into Public Goods Treasury: $%.2f USDC",
            g_coordinator.chain.public_goods_treasury);
    gui_log("    - PCO Forced Sale Triggered: Evaluator NFT #1001 acquired by %s for $%.1f USDC",
            new_bidder, purchase_bid);

    update_participants_table();
    update_evaluators_table();
    InvalidateRect(g_hMainWnd, NULL, TRUE);
}

static void on_action_wallet(void)
{
    if (g_coordinator.wallet_mgr.count == 0)
    {
        din_wallet_t w;
        char label[32];
        snprintf(label, sizeof(label), "Sovereign_Wallet_%02u", g_coordinator.wallet_mgr.count + 1);
        din_error_t err = din_wallet_create(&g_coordinator.wallet_mgr, label, DIN_ROLE_PARTICIPANT, &w);
        if (err == DIN_OK)
        {
            din_wallet_fund(&g_coordinator.wallet_mgr, w.address.str, 1000.0f, 1.5f);
        }
    }
    din_wallet_gui_open(g_hMainWnd, &g_coordinator.wallet_mgr);
    gui_log("[Wallet UI] Opened Sovereign Wallet Manager & Key Vault.");
}

static void on_action_autorun(void)
{
    gui_log(">>> RUNNING COMPLETE AUTOMATED DIN LIFECYCLE <<<");
    on_action_init();
    on_action_enroll();
    for (uint32_t r = 0; r < 3; ++r)
    {
        on_action_round();
    }
    on_action_audit();
    on_action_settle();
    gui_log(">>> AUTOMATED SIMULATION COMPLETED SUCCESSFULLY <<<");
}

static void on_action_reset(void)
{
    g_initialized = false;
    g_enrolled = false;
    g_audited = false;
    g_settled = false;
    g_rounds_completed = 0;
    memset(&g_coordinator, 0, sizeof(din_coordinator_t));
    SetWindowTextA(g_hEditLog, "");
    ListView_DeleteAllItems(g_hLvParticipants);
    ListView_DeleteAllItems(g_hLvEvaluators);
    gui_log(">>> System reset. Ready for new DIN network initialization.");
    InvalidateRect(g_hMainWnd, NULL, TRUE);
}

static void draw_stat_card(HDC hdc, RECT rect, const char *title, const char *value, COLORREF val_color)
{
    FillRect(hdc, &rect, g_hBrushCard);
    FrameRect(hdc, &rect, g_hBrushBorder);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, COLOR_TEXT_MUTED);
    SelectObject(hdc, g_hFontBody);
    RECT rTitle = { rect.left + 10, rect.top + 8, rect.right - 10, rect.top + 26 };
    DrawTextA(hdc, title, -1, &rTitle, DT_LEFT | DT_SINGLELINE);

    SetTextColor(hdc, val_color);
    SelectObject(hdc, g_hFontTitle);
    RECT rVal = { rect.left + 10, rect.top + 28, rect.right - 10, rect.bottom - 8 };
    DrawTextA(hdc, value, -1, &rVal, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
}

static void on_paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT client;
    GetClientRect(hwnd, &client);

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBm = CreateCompatibleBitmap(hdc, client.right, client.bottom);
    HGDIOBJ oldBm = SelectObject(memDC, memBm);

    FillRect(memDC, &client, g_hBrushBg);

    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, COLOR_TEXT_PRIMARY);
    SelectObject(memDC, g_hFontTitle);
    TextOutA(memDC, 20, 15, "Decentralized Intelligence Network (DIN) - Sovereign AI & Auditing Console", 74);

    SetTextColor(memDC, COLOR_TEXT_MUTED);
    SelectObject(memDC, g_hFontBody);
    TextOutA(memDC, 20, 42, "Ox0 | 0x0 | 0x0 | 0x0", 21);

    const char *phase_str = "UNINITIALIZED";
    COLORREF phase_col = COLOR_TEXT_MUTED;
    if (g_settled)
    {
        phase_str = "PHASE: REWARDS SETTLED & COMPLETED";
        phase_col = COLOR_SUCCESS;
    }
    else if (g_audited)
    {
        phase_str = "PHASE: AUDITING & BYZANTINE DETECTION";
        phase_col = COLOR_WARNING;
    }
    else if (g_rounds_completed > 0)
    {
        phase_str = "PHASE: FEDERATED LEARNING IN PROGRESS";
        phase_col = COLOR_ACCENT;
    }
    else if (g_enrolled)
    {
        phase_str = "PHASE: NODES ENROLLED (PDS & NFTS)";
        phase_col = COLOR_ACCENT;
    }
    else if (g_initialized)
    {
        phase_str = "PHASE: GENESIS MODEL PUBLISHED";
        phase_col = COLOR_SUCCESS;
    }

    SetTextColor(memDC, phase_col);
    SelectObject(memDC, g_hFontBody);
    RECT rPhase = { client.right - 350, 20, client.right - 20, 45 };
    DrawTextA(memDC, phase_str, -1, &rPhase, DT_RIGHT | DT_SINGLELINE);

    char val_escrow[32] = "$0.00 USDC";
    char val_treasury[32] = "$0.00 USDC";
    char val_round[32] = "Round 0";
    char val_cid[64] = "None";

    if (g_initialized)
    {
        snprintf(val_escrow, sizeof(val_escrow), "$%.2f USDC", g_coordinator.chain.reward_escrow_pool);
        snprintf(val_treasury, sizeof(val_treasury), "$%.2f USDC", g_coordinator.chain.public_goods_treasury);
        snprintf(val_round, sizeof(val_round), "Round %u", g_rounds_completed);
        snprintf(val_cid, sizeof(val_cid), "%.18s...", g_coordinator.chain.current_global_cid.str);
    }

    RECT rCard1 = { 20, 75, 230, 140 };
    RECT rCard2 = { 245, 75, 455, 140 };
    RECT rCard3 = { 470, 75, 680, 140 };
    RECT rCard4 = { 695, 75, 905, 140 };
    RECT rCard5 = { 920, 75, client.right - 20, 140 };

    draw_stat_card(memDC, rCard1, "ESCROW VAULT POOL", val_escrow, COLOR_ACCENT);
    draw_stat_card(memDC, rCard2, "PUBLIC GOODS TREASURY", val_treasury, COLOR_SUCCESS);
    draw_stat_card(memDC, rCard3, "FL TRAINING ROUND", val_round, COLOR_TEXT_PRIMARY);
    draw_stat_card(memDC, rCard4, "GLOBAL MODEL CID", val_cid, COLOR_TEXT_PRIMARY);
    draw_stat_card(memDC, rCard5, "SOVEREIGN STATUS", g_enrolled ? "10 PDS / 3 Evaluators" : "Standby", COLOR_ACCENT);

    BitBlt(hdc, 0, 0, client.right, client.bottom, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBm);
    DeleteObject(memBm);
    DeleteDC(memDC);
    EndPaint(hwnd, &ps);
}

static void create_list_columns(void)
{
    LVCOLUMNA lvc;
    memset(&lvc, 0, sizeof(lvc));
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    lvc.pszText = "ID";
    lvc.cx = 50;
    ListView_InsertColumn(g_hLvParticipants, 0, &lvc);
    lvc.pszText = "Address";
    lvc.cx = 170;
    ListView_InsertColumn(g_hLvParticipants, 1, &lvc);
    lvc.pszText = "Data Store (PDS)";
    lvc.cx = 130;
    ListView_InsertColumn(g_hLvParticipants, 2, &lvc);
    lvc.pszText = "Node Status";
    lvc.cx = 110;
    ListView_InsertColumn(g_hLvParticipants, 3, &lvc);
    lvc.pszText = "Uploaded Model CID";
    lvc.cx = 160;
    ListView_InsertColumn(g_hLvParticipants, 4, &lvc);
    lvc.pszText = "Audit Score";
    lvc.cx = 90;
    ListView_InsertColumn(g_hLvParticipants, 5, &lvc);
    lvc.pszText = "Reward Payout";
    lvc.cx = 110;
    ListView_InsertColumn(g_hLvParticipants, 6, &lvc);

    lvc.pszText = "NFT Token";
    lvc.cx = 90;
    ListView_InsertColumn(g_hLvEvaluators, 0, &lvc);
    lvc.pszText = "Evaluator Address";
    lvc.cx = 190;
    ListView_InsertColumn(g_hLvEvaluators, 1, &lvc);
    lvc.pszText = "Self Valuation";
    lvc.cx = 110;
    ListView_InsertColumn(g_hLvEvaluators, 2, &lvc);
    lvc.pszText = "Harberger Tax";
    lvc.cx = 90;
    ListView_InsertColumn(g_hLvEvaluators, 3, &lvc);
    lvc.pszText = "Tax Paid";
    lvc.cx = 90;
    ListView_InsertColumn(g_hLvEvaluators, 4, &lvc);
    lvc.pszText = "Reward Payout";
    lvc.cx = 110;
    ListView_InsertColumn(g_hLvEvaluators, 5, &lvc);
    lvc.pszText = "Status";
    lvc.cx = 140;
    ListView_InsertColumn(g_hLvEvaluators, 6, &lvc);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        InitCommonControls();
        g_hFontTitle = CreateFontA(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        g_hFontBody = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        g_hFontMono = CreateFontA(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");

        g_hBrushBg = CreateSolidBrush(COLOR_BG);
        g_hBrushCard = CreateSolidBrush(COLOR_CARD);
        g_hBrushBorder = CreateSolidBrush(COLOR_BORDER);

        int bx = 20;
        int by = 155;
        int bw = 125;
        int bh = 32;
        int gap = 10;

        g_hBtnInit = CreateWindowA("BUTTON", "1. Init & Escrow", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, bx, by, bw, bh, hwnd, (HMENU)ID_BTN_INIT, NULL, NULL);
        g_hBtnEnroll = CreateWindowA("BUTTON", "2. Enroll Nodes", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, bx + (bw + gap) * 1, by, bw, bh, hwnd, (HMENU)ID_BTN_ENROLL, NULL, NULL);
        g_hBtnRound = CreateWindowA("BUTTON", "3. Run FL Round", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, bx + (bw + gap) * 2, by, bw, bh, hwnd, (HMENU)ID_BTN_ROUND, NULL, NULL);
        g_hBtnAudit = CreateWindowA("BUTTON", "4. Run Audit", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, bx + (bw + gap) * 3, by, bw, bh, hwnd, (HMENU)ID_BTN_AUDIT, NULL, NULL);
        g_hBtnSettle = CreateWindowA("BUTTON", "5. Settle & Tax", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, bx + (bw + gap) * 4, by, bw, bh, hwnd, (HMENU)ID_BTN_SETTLE, NULL, NULL);
        g_hBtnWallet = CreateWindowA("BUTTON", "Wallet Vault UI", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, bx + (bw + gap) * 5, by, bw, bh, hwnd, (HMENU)ID_BTN_WALLET, NULL, NULL);
        g_hBtnAutoRun = CreateWindowA("BUTTON", "Run Full Flow", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, bx + (bw + gap) * 6, by, bw, bh, hwnd, (HMENU)ID_BTN_AUTORUN, NULL, NULL);
        g_hBtnReset = CreateWindowA("BUTTON", "Reset", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, bx + (bw + gap) * 7, by, 95, bh, hwnd, (HMENU)ID_BTN_RESET, NULL, NULL);

        SendMessage(g_hBtnInit, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        SendMessage(g_hBtnEnroll, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        SendMessage(g_hBtnRound, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        SendMessage(g_hBtnAudit, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        SendMessage(g_hBtnSettle, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        SendMessage(g_hBtnWallet, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        SendMessage(g_hBtnAutoRun, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        SendMessage(g_hBtnReset, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hLvParticipants = CreateWindowExA(0, WC_LISTVIEWA, "", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | WS_BORDER, 20, 200, 830, 240, hwnd, (HMENU)ID_LV_PARTICIPANTS, NULL, NULL);
        ListView_SetExtendedListViewStyle(g_hLvParticipants, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        SendMessage(g_hLvParticipants, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hLvEvaluators = CreateWindowExA(0, WC_LISTVIEWA, "", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | WS_BORDER, 20, 455, 830, 220, hwnd, (HMENU)ID_LV_EVALUATORS, NULL, NULL);
        ListView_SetExtendedListViewStyle(g_hLvEvaluators, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        SendMessage(g_hLvEvaluators, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hEditLog = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 865, 200, 310, 475, hwnd, (HMENU)ID_EDIT_LOG, NULL, NULL);
        SendMessage(g_hEditLog, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);

        create_list_columns();
        gui_log("DIN Native Win32 Console Ready.");
        gui_log("Click '1. Init & Escrow' to begin.");
        return 0;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case ID_BTN_INIT:
            on_action_init();
            break;
        case ID_BTN_ENROLL:
            on_action_enroll();
            break;
        case ID_BTN_ROUND:
            on_action_round();
            break;
        case ID_BTN_AUDIT:
            on_action_audit();
            break;
        case ID_BTN_SETTLE:
            on_action_settle();
            break;
        case ID_BTN_WALLET:
            on_action_wallet();
            break;
        case ID_BTN_AUTORUN:
            on_action_autorun();
            break;
        case ID_BTN_RESET:
            on_action_reset();
            break;
        }
        return 0;
    }
    case WM_SIZE:
    {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        if (g_hLvParticipants && g_hLvEvaluators && g_hEditLog)
        {
            int log_w = (w > 1100) ? 380 : 310;
            int table_w = w - log_w - 50;
            if (table_w > 400)
            {
                int top_table_h = (h - 220) / 2 - 10;
                MoveWindow(g_hLvParticipants, 20, 200, table_w, top_table_h, TRUE);
                MoveWindow(g_hLvEvaluators, 20, 200 + top_table_h + 15, table_w, top_table_h, TRUE);
                MoveWindow(g_hEditLog, 30 + table_w, 200, log_w, top_table_h * 2 + 15, TRUE);
            }
        }
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }
    case WM_ERASEBKGND:
    {
        return 1;
    }
    case WM_PAINT:
    {
        on_paint(hwnd);
        return 0;
    }
    case WM_DESTROY:
    {
        if (g_hFontTitle) DeleteObject(g_hFontTitle);
        if (g_hFontBody) DeleteObject(g_hFontBody);
        if (g_hFontMono) DeleteObject(g_hFontMono);
        if (g_hBrushBg) DeleteObject(g_hBrushBg);
        if (g_hBrushCard) DeleteObject(g_hBrushCard);
        if (g_hBrushBorder) DeleteObject(g_hBrushBorder);
        PostQuitMessage(0);
        return 0;
    }
    default:
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
}

din_error_t din_gui_init(HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASSEXA wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "DIN_WinApi_GUI_Class";

    if (!RegisterClassExA(&wc))
    {
        return DIN_ERR_INVALID_STATE;
    }
    g_hMainWnd = CreateWindowExA(0, "DIN_WinApi_GUI_Class", "Decentralized Intelligence Network (DIN) - Native Windows Console", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1220, 740, NULL, NULL, hInstance, NULL);
    if (!g_hMainWnd)
    {
        return DIN_ERR_INVALID_STATE;
    }
    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);
    return DIN_OK;
}

int din_gui_run_message_loop(void)
{
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return (int)msg.wParam;
}
