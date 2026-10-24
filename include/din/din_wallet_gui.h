#ifndef DIN_WALLET_GUI_H
#define DIN_WALLET_GUI_H

#include "din_types.h"
#include "din_wallet.h"
#include <windows.h>

din_error_t din_wallet_gui_open(HWND hParent, din_wallet_manager_t *mgr);
din_error_t din_wallet_gui_create_standalone(HINSTANCE hInstance, int nCmdShow, din_wallet_manager_t *mgr);

#endif
