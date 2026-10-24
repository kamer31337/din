#ifndef DIN_GUI_H
#define DIN_GUI_H

#include "din_types.h"
#include <windows.h>

din_error_t din_gui_init(HINSTANCE hInstance, int nCmdShow);
int din_gui_run_message_loop(void);

#endif
