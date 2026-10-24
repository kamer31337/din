#include "din/din_gui.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    din_error_t err = din_gui_init(hInstance, nCmdShow);
    if (err != DIN_OK)
    {
        return 1;
    }
    return din_gui_run_message_loop();
}

int main(void)
{
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWDEFAULT);
}
