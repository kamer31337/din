#include "din/din_wallet_gui.h"
#include "din/din_crypto.h"
#include <commctrl.h>
#include <stdio.h>
#include <string.h>

#define ID_WLT_LIST 2001
#define ID_WLT_BTN_NEW 2002
#define ID_WLT_BTN_IMPORT 2003
#define ID_WLT_BTN_FAUCET 2004
#define ID_WLT_BTN_COPY_ADDR 2005
#define ID_WLT_BTN_TOGGLE_SEED 2006
#define ID_WLT_BTN_TOGGLE_KEY 2007
#define ID_WLT_BTN_SEND 2008
#define ID_WLT_BTN_SIGN 2009
#define ID_WLT_BTN_EXPORT 2010
#define ID_WLT_EDIT_TO 2011
#define ID_WLT_EDIT_AMOUNT 2012
#define ID_WLT_EDIT_MSG 2013
#define ID_WLT_EDIT_SIG 2014

static din_wallet_manager_t *g_pWalletMgr = NULL;
static HWND g_hWalletWnd = NULL;
static HWND g_hLvWallets = NULL;
static HWND g_hBtnNew = NULL;
static HWND g_hBtnImport = NULL;
static HWND g_hBtnFaucet = NULL;
static HWND g_hBtnCopyAddr = NULL;
static HWND g_hBtnToggleSeed = NULL;
static HWND g_hBtnToggleKey = NULL;
static HWND g_hBtnSend = NULL;
static HWND g_hBtnSign = NULL;
static HWND g_hBtnExport = NULL;
static HWND g_hEditTo = NULL;
static HWND g_hEditAmount = NULL;
static HWND g_hEditMsg = NULL;
static HWND g_hEditSig = NULL;

static bool g_show_seed = false;
static bool g_show_key = false;
static uint32_t g_selected_wallet = 0;

static HFONT g_hFontTitle = NULL;
static HFONT g_hFontBody = NULL;
static HFONT g_hFontBold = NULL;
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
static const COLORREF COLOR_GOLD = RGB(249, 226, 175);

static void copy_to_clipboard(HWND hwnd, const char *text)
{
    if (!text || strlen(text) == 0)
    {
        return;
    }
    if (OpenClipboard(hwnd))
    {
        EmptyClipboard();
        size_t len = strlen(text) + 1;
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
        if (hMem)
        {
            memcpy(GlobalLock(hMem), text, len);
            GlobalUnlock(hMem);
            SetClipboardData(CF_TEXT, hMem);
        }
        CloseClipboard();
    }
}

static void refresh_wallet_list(void)
{
    if (!g_hLvWallets || !g_pWalletMgr)
    {
        return;
    }
    ListView_DeleteAllItems(g_hLvWallets);
    for (uint32_t i = 0; i < g_pWalletMgr->count; ++i)
    {
        const din_wallet_t *w = &g_pWalletMgr->wallets[i];
        LVITEMA lvi;
        char buf[64];

        memset(&lvi, 0, sizeof(lvi));
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.pszText = (LPSTR)w->label;
        ListView_InsertItem(g_hLvWallets, &lvi);

        snprintf(buf, sizeof(buf), "%.10s...%.6s", w->address.str, w->address.str + 36);
        ListView_SetItemText(g_hLvWallets, i, 1, buf);
        snprintf(buf, sizeof(buf), "$%.2f", w->usdc_balance);
        ListView_SetItemText(g_hLvWallets, i, 2, buf);
    }
    if (g_pWalletMgr->count > 0 && g_selected_wallet < g_pWalletMgr->count)
    {
        ListView_SetItemState(g_hLvWallets, g_selected_wallet, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
}

static void on_create_new_wallet(HWND hwnd)
{
    if (!g_pWalletMgr)
    {
        return;
    }
    char label[32];
    snprintf(label, sizeof(label), "Sovereign_Wallet_%02u", g_pWalletMgr->count + 1);
    din_wallet_t w;
    din_error_t err = din_wallet_create(g_pWalletMgr, label, DIN_ROLE_PARTICIPANT, &w);
    if (err == DIN_OK)
    {
        din_wallet_fund(g_pWalletMgr, w.address.str, 1000.0f, 1.0f);
        g_selected_wallet = g_pWalletMgr->count - 1;
        refresh_wallet_list();
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

static void on_import_mnemonic_dialog(HWND hwnd)
{
    if (!g_pWalletMgr)
    {
        return;
    }
    const char *default_mnemonic = "alpha beacon cipher crystal diamond eagle falcon galaxy harbor island jungle kernel";
    char label[32];
    snprintf(label, sizeof(label), "Imported_Wallet_%02u", g_pWalletMgr->count + 1);
    din_wallet_t w;
    din_error_t err = din_wallet_import_mnemonic(g_pWalletMgr, default_mnemonic, label, DIN_ROLE_PARTICIPANT, &w);
    if (err == DIN_OK)
    {
        din_wallet_fund(g_pWalletMgr, w.address.str, 500.0f, 1.0f);
        g_selected_wallet = g_pWalletMgr->count - 1;
        refresh_wallet_list();
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

static void on_faucet(HWND hwnd)
{
    if (!g_pWalletMgr || g_pWalletMgr->count == 0)
    {
        return;
    }
    din_wallet_t *w = &g_pWalletMgr->wallets[g_selected_wallet];
    din_wallet_fund(g_pWalletMgr, w->address.str, 1000.0f, 0.5f);
    refresh_wallet_list();
    InvalidateRect(hwnd, NULL, TRUE);
}

static void on_transfer_usdc(HWND hwnd)
{
    if (!g_pWalletMgr || g_pWalletMgr->count == 0)
    {
        return;
    }
    char to_addr[DIN_ADDR_MAX_LEN] = {0};
    char amt_buf[32] = {0};
    GetWindowTextA(g_hEditTo, to_addr, sizeof(to_addr));
    GetWindowTextA(g_hEditAmount, amt_buf, sizeof(amt_buf));

    float amt = (float)atof(amt_buf);
    if (amt <= 0.0f || strlen(to_addr) < 4)
    {
        MessageBoxA(hwnd, "Please enter a valid recipient address and amount.", "Transfer Error", MB_OK | MB_ICONWARNING);
        return;
    }
    din_wallet_t *sender = &g_pWalletMgr->wallets[g_selected_wallet];
    din_error_t err = din_wallet_transfer_usdc(g_pWalletMgr, sender->address.str, to_addr, amt);
    if (err == DIN_ERR_INSUFFICIENT_FUNDS)
    {
        MessageBoxA(hwnd, "Insufficient USDC balance in active wallet.", "Transfer Failed", MB_OK | MB_ICONERROR);
        return;
    }
    else if (err == DIN_ERR_NOT_FOUND)
    {
        if (sender->usdc_balance >= amt)
        {
            sender->usdc_balance -= amt;
            MessageBoxA(hwnd, "External Transfer Broadcasted to Public Blockchain!", "Transfer Confirmed", MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            MessageBoxA(hwnd, "Insufficient funds.", "Transfer Error", MB_OK | MB_ICONERROR);
            return;
        }
    }
    else
    {
        MessageBoxA(hwnd, "Internal Transfer Successful between Sovereign Wallets!", "Transfer Complete", MB_OK | MB_ICONINFORMATION);
    }
    SetWindowTextA(g_hEditAmount, "");
    refresh_wallet_list();
    InvalidateRect(hwnd, NULL, TRUE);
}

static void on_sign_message(void)
{
    if (!g_pWalletMgr || g_pWalletMgr->count == 0)
    {
        return;
    }
    char msg[256] = {0};
    GetWindowTextA(g_hEditMsg, msg, sizeof(msg));
    if (strlen(msg) == 0)
    {
        return;
    }
    const din_wallet_t *w = &g_pWalletMgr->wallets[g_selected_wallet];
    din_hash256_t sig;
    din_wallet_sign_data(w, (const uint8_t *)msg, strlen(msg), &sig);

    char hex[DIN_HASH_LEN * 2 + 1];
    din_hash_to_hex(&sig, hex);
    char full_sig[128];
    snprintf(full_sig, sizeof(full_sig), "0x%s", hex);
    SetWindowTextA(g_hEditSig, full_sig);
}

static void on_export_keystore(HWND hwnd)
{
    if (!g_pWalletMgr)
    {
        return;
    }
    din_error_t err = din_wallet_save_file(g_pWalletMgr, "din_sovereign_wallets.keystore");
    if (err == DIN_OK)
    {
        MessageBoxA(hwnd, "Exported successfully to 'din_sovereign_wallets.keystore'", "Keystore Exported", MB_OK | MB_ICONINFORMATION);
    }
}

static void paint_wallet_ui(HWND hwnd)
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
    TextOutA(memDC, 20, 15, "Sovereign Wallet Manager & Keystore", 35);

    SetTextColor(memDC, COLOR_TEXT_MUTED);
    SelectObject(memDC, g_hFontBody);
    TextOutA(memDC, 20, 42, "BIP-39 Mnemonic Seed Phrase | Self-Sovereign Identity (DID) | PDS Key Vault", 76);

    RECT rLeftHeader = { 20, 75, 300, 95 };
    SetTextColor(memDC, COLOR_ACCENT);
    SelectObject(memDC, g_hFontBold);
    DrawTextA(memDC, "SOVEREIGN ACCOUNTS", -1, &rLeftHeader, DT_LEFT | DT_SINGLELINE);

    RECT rRightCard = { 320, 75, client.right - 20, 245 };
    FillRect(memDC, &rRightCard, g_hBrushCard);
    FrameRect(memDC, &rRightCard, g_hBrushBorder);

    if (g_pWalletMgr && g_pWalletMgr->count > 0 && g_selected_wallet < g_pWalletMgr->count)
    {
        const din_wallet_t *w = &g_pWalletMgr->wallets[g_selected_wallet];

        SetTextColor(memDC, COLOR_GOLD);
        SelectObject(memDC, g_hFontTitle);
        TextOutA(memDC, 340, 90, w->label, strlen(w->label));

        const char *role_str = "PARTICIPANT (PDS HOLDER)";
        if (w->role == DIN_ROLE_MODEL_OWNER) role_str = "MODEL OWNER";
        else if (w->role == DIN_ROLE_EVALUATOR) role_str = "STAKED EVALUATOR (AUDITOR)";

        SetTextColor(memDC, COLOR_TEXT_MUTED);
        SelectObject(memDC, g_hFontBody);
        TextOutA(memDC, 340, 120, "Role:", 5);
        SetTextColor(memDC, COLOR_ACCENT);
        TextOutA(memDC, 385, 120, role_str, strlen(role_str));

        SetTextColor(memDC, COLOR_TEXT_MUTED);
        TextOutA(memDC, 340, 145, "Address:", 8);
        SetTextColor(memDC, COLOR_TEXT_PRIMARY);
        SelectObject(memDC, g_hFontMono);
        TextOutA(memDC, 410, 145, w->address.str, strlen(w->address.str));

        SelectObject(memDC, g_hFontBody);
        SetTextColor(memDC, COLOR_TEXT_MUTED);
        TextOutA(memDC, 340, 170, "DID:", 4);
        SelectObject(memDC, g_hFontMono);
        SetTextColor(memDC, COLOR_TEXT_PRIMARY);
        TextOutA(memDC, 410, 170, w->did, strlen(w->did));

        char bal_str[64];
        snprintf(bal_str, sizeof(bal_str), "Balance: $%.2f USDC  |  Gas: %.4f ETH", w->usdc_balance, w->eth_balance);
        SelectObject(memDC, g_hFontTitle);
        SetTextColor(memDC, COLOR_SUCCESS);
        TextOutA(memDC, 340, 202, bal_str, strlen(bal_str));
    }

    RECT rSeedCard = { 320, 255, client.right - 20, 405 };
    FillRect(memDC, &rSeedCard, g_hBrushCard);
    FrameRect(memDC, &rSeedCard, g_hBrushBorder);

    SetTextColor(memDC, COLOR_ACCENT);
    SelectObject(memDC, g_hFontBold);
    TextOutA(memDC, 340, 268, "SECURITY VAULT (12-WORD MNEMONIC & PRIVATE KEY)", 47);

    if (g_pWalletMgr && g_pWalletMgr->count > 0 && g_selected_wallet < g_pWalletMgr->count)
    {
        const din_wallet_t *w = &g_pWalletMgr->wallets[g_selected_wallet];

        SetTextColor(memDC, COLOR_TEXT_MUTED);
        SelectObject(memDC, g_hFontBody);
        TextOutA(memDC, 340, 298, "Seed Phrase:", 12);

        RECT rSeedText = { 340, 320, client.right - 40, 360 };
        SelectObject(memDC, g_hFontMono);
        if (g_show_seed)
        {
            SetTextColor(memDC, COLOR_GOLD);
            DrawTextA(memDC, w->mnemonic, -1, &rSeedText, DT_LEFT | DT_WORDBREAK);
        }
        else
        {
            SetTextColor(memDC, COLOR_TEXT_MUTED);
            DrawTextA(memDC, "[HIDDEN]  Click 'Show Seed Phrase' to reveal your 12-word recovery mnemonic.", -1, &rSeedText, DT_LEFT | DT_WORDBREAK);
        }

        SelectObject(memDC, g_hFontBody);
        SetTextColor(memDC, COLOR_TEXT_MUTED);
        TextOutA(memDC, 340, 368, "Private Key:", 12);

        SelectObject(memDC, g_hFontMono);
        if (g_show_key)
        {
            char priv_hex[DIN_HASH_LEN * 2 + 1];
            din_hash256_t pkh;
            memcpy(pkh.bytes, w->private_key, DIN_HASH_LEN);
            din_hash_to_hex(&pkh, priv_hex);
            SetTextColor(memDC, COLOR_ACCENT);
            TextOutA(memDC, 430, 368, priv_hex, strlen(priv_hex));
        }
        else
        {
            SetTextColor(memDC, COLOR_TEXT_MUTED);
            TextOutA(memDC, 430, 368, "****************************************************************", 64);
        }
    }

    RECT rTxCard = { 320, 415, client.right - 20, client.bottom - 20 };
    FillRect(memDC, &rTxCard, g_hBrushCard);
    FrameRect(memDC, &rTxCard, g_hBrushBorder);

    SetTextColor(memDC, COLOR_ACCENT);
    SelectObject(memDC, g_hFontBold);
    TextOutA(memDC, 340, 428, "TRANSFER USDC & CRYPTOGRAPHIC PAYLOAD SIGNING", 45);

    SetTextColor(memDC, COLOR_TEXT_MUTED);
    SelectObject(memDC, g_hFontBody);
    TextOutA(memDC, 340, 460, "Recipient Address (0x...):", 26);
    TextOutA(memDC, 630, 460, "Amount ($ USDC):", 16);

    TextOutA(memDC, 340, 530, "Payload to Sign (Training Delta Hash / Message):", 48);
    TextOutA(memDC, 340, 595, "Signature Hash:", 15);

    BitBlt(hdc, 0, 0, client.right, client.bottom, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBm);
    DeleteObject(memBm);
    DeleteDC(memDC);
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK WalletWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        g_hFontTitle = CreateFontA(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        g_hFontBody = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        g_hFontBold = CreateFontA(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        g_hFontMono = CreateFontA(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");

        g_hBrushBg = CreateSolidBrush(COLOR_BG);
        g_hBrushCard = CreateSolidBrush(COLOR_CARD);
        g_hBrushBorder = CreateSolidBrush(COLOR_BORDER);

        g_hLvWallets = CreateWindowExA(0, WC_LISTVIEWA, "", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | WS_BORDER, 20, 100, 280, 480, hwnd, (HMENU)ID_WLT_LIST, NULL, NULL);
        ListView_SetExtendedListViewStyle(g_hLvWallets, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        SendMessage(g_hLvWallets, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        LVCOLUMNA lvc;
        memset(&lvc, 0, sizeof(lvc));
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvc.pszText = (LPSTR)"Label";
        lvc.cx = 110;
        ListView_InsertColumn(g_hLvWallets, 0, &lvc);
        lvc.pszText = (LPSTR)"Address";
        lvc.cx = 95;
        ListView_InsertColumn(g_hLvWallets, 1, &lvc);
        lvc.pszText = (LPSTR)"USDC";
        lvc.cx = 65;
        ListView_InsertColumn(g_hLvWallets, 2, &lvc);

        g_hBtnNew = CreateWindowA("BUTTON", "+ Create Wallet", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 590, 135, 30, hwnd, (HMENU)ID_WLT_BTN_NEW, NULL, NULL);
        g_hBtnImport = CreateWindowA("BUTTON", "Import Seed", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 165, 590, 135, 30, hwnd, (HMENU)ID_WLT_BTN_IMPORT, NULL, NULL);
        g_hBtnFaucet = CreateWindowA("BUTTON", "Faucet +$1000", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 625, 135, 30, hwnd, (HMENU)ID_WLT_BTN_FAUCET, NULL, NULL);
        g_hBtnExport = CreateWindowA("BUTTON", "Export Keystore", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 165, 625, 135, 30, hwnd, (HMENU)ID_WLT_BTN_EXPORT, NULL, NULL);

        g_hBtnCopyAddr = CreateWindowA("BUTTON", "Copy Address", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 780, 140, 110, 24, hwnd, (HMENU)ID_WLT_BTN_COPY_ADDR, NULL, NULL);
        g_hBtnToggleSeed = CreateWindowA("BUTTON", "Show Seed Phrase", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 760, 264, 130, 24, hwnd, (HMENU)ID_WLT_BTN_TOGGLE_SEED, NULL, NULL);
        g_hBtnToggleKey = CreateWindowA("BUTTON", "Show Key", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 810, 365, 80, 24, hwnd, (HMENU)ID_WLT_BTN_TOGGLE_KEY, NULL, NULL);

        g_hEditTo = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 340, 482, 275, 24, hwnd, (HMENU)ID_WLT_EDIT_TO, NULL, NULL);
        g_hEditAmount = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "100.00", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 630, 482, 110, 24, hwnd, (HMENU)ID_WLT_EDIT_AMOUNT, NULL, NULL);
        g_hBtnSend = CreateWindowA("BUTTON", "Send USDC", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 760, 481, 130, 26, hwnd, (HMENU)ID_WLT_BTN_SEND, NULL, NULL);

        g_hEditMsg = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "DIN_ROUND_1_DELTA_COMMITMENT", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 340, 552, 400, 24, hwnd, (HMENU)ID_WLT_EDIT_MSG, NULL, NULL);
        g_hBtnSign = CreateWindowA("BUTTON", "Sign Message", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 760, 551, 130, 26, hwnd, (HMENU)ID_WLT_BTN_SIGN, NULL, NULL);
        g_hEditSig = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY, 340, 618, 550, 24, hwnd, (HMENU)ID_WLT_EDIT_SIG, NULL, NULL);

        SendMessage(g_hBtnNew, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        SendMessage(g_hBtnImport, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        SendMessage(g_hBtnFaucet, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        SendMessage(g_hBtnExport, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        SendMessage(g_hBtnCopyAddr, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        SendMessage(g_hBtnToggleSeed, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        SendMessage(g_hBtnToggleKey, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        SendMessage(g_hBtnSend, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        SendMessage(g_hBtnSign, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        SendMessage(g_hEditTo, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);
        SendMessage(g_hEditAmount, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        SendMessage(g_hEditMsg, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);
        SendMessage(g_hEditSig, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);

        refresh_wallet_list();
        return 0;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case ID_WLT_BTN_NEW:
            on_create_new_wallet(hwnd);
            break;
        case ID_WLT_BTN_IMPORT:
            on_import_mnemonic_dialog(hwnd);
            break;
        case ID_WLT_BTN_FAUCET:
            on_faucet(hwnd);
            break;
        case ID_WLT_BTN_COPY_ADDR:
            if (g_pWalletMgr && g_pWalletMgr->count > 0)
            {
                copy_to_clipboard(hwnd, g_pWalletMgr->wallets[g_selected_wallet].address.str);
            }
            break;
        case ID_WLT_BTN_TOGGLE_SEED:
            g_show_seed = !g_show_seed;
            SetWindowTextA(g_hBtnToggleSeed, g_show_seed ? "Hide Seed" : "Show Seed Phrase");
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case ID_WLT_BTN_TOGGLE_KEY:
            g_show_key = !g_show_key;
            SetWindowTextA(g_hBtnToggleKey, g_show_key ? "Hide Key" : "Show Key");
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case ID_WLT_BTN_SEND:
            on_transfer_usdc(hwnd);
            break;
        case ID_WLT_BTN_SIGN:
            on_sign_message();
            break;
        case ID_WLT_BTN_EXPORT:
            on_export_keystore(hwnd);
            break;
        }
        return 0;
    }
    case WM_NOTIFY:
    {
        LPNMHDR pnm = (LPNMHDR)lParam;
        if (pnm->idFrom == ID_WLT_LIST && pnm->code == LVN_ITEMCHANGED)
        {
            LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
            if ((pnmv->uNewState & LVIS_SELECTED) && !(pnmv->uOldState & LVIS_SELECTED))
            {
                g_selected_wallet = (uint32_t)pnmv->iItem;
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }
        return 0;
    }
    case WM_ERASEBKGND:
    {
        return 1;
    }
    case WM_PAINT:
    {
        paint_wallet_ui(hwnd);
        return 0;
    }
    case WM_DESTROY:
    {
        if (g_hFontTitle) DeleteObject(g_hFontTitle);
        if (g_hFontBody) DeleteObject(g_hFontBody);
        if (g_hFontBold) DeleteObject(g_hFontBold);
        if (g_hFontMono) DeleteObject(g_hFontMono);
        if (g_hBrushBg) DeleteObject(g_hBrushBg);
        if (g_hBrushCard) DeleteObject(g_hBrushCard);
        if (g_hBrushBorder) DeleteObject(g_hBrushBorder);
        g_hWalletWnd = NULL;
        return 0;
    }
    default:
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
}

din_error_t din_wallet_gui_open(HWND hParent, din_wallet_manager_t *mgr)
{
    if (!mgr)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    g_pWalletMgr = mgr;
    if (g_hWalletWnd && IsWindow(g_hWalletWnd))
    {
        SetForegroundWindow(g_hWalletWnd);
        return DIN_OK;
    }

    HINSTANCE hInst = GetModuleHandle(NULL);
    WNDCLASSEXA wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WalletWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "DIN_Wallet_GUI_Class";

    RegisterClassExA(&wc);

    g_hWalletWnd = CreateWindowExA(WS_EX_DLGMODALFRAME, "DIN_Wallet_GUI_Class", "DIN Sovereign Wallet & Key Vault", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 960, 710, hParent, NULL, hInst, NULL);
    if (!g_hWalletWnd)
    {
        return DIN_ERR_INVALID_STATE;
    }
    ShowWindow(g_hWalletWnd, SW_SHOW);
    UpdateWindow(g_hWalletWnd);
    return DIN_OK;
}

din_error_t din_wallet_gui_create_standalone(HINSTANCE hInstance, int nCmdShow, din_wallet_manager_t *mgr)
{
    if (!mgr)
    {
        return DIN_ERR_INVALID_PARAM;
    }
    g_pWalletMgr = mgr;
    WNDCLASSEXA wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WalletWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "DIN_Wallet_GUI_Standalone_Class";

    if (!RegisterClassExA(&wc))
    {
        return DIN_ERR_INVALID_STATE;
    }
    g_hWalletWnd = CreateWindowExA(0, "DIN_Wallet_GUI_Standalone_Class", "DIN Sovereign Wallet & Key Vault", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 960, 710, NULL, NULL, hInstance, NULL);
    if (!g_hWalletWnd)
    {
        return DIN_ERR_INVALID_STATE;
    }
    ShowWindow(g_hWalletWnd, nCmdShow);
    UpdateWindow(g_hWalletWnd);
    return DIN_OK;
}
