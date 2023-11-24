#define BASE_IMPLEMENTATION
#include "base.h"

#include <windows.h>

int CALLBACK WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    MessageBox(0, "This is nbody2", "nbody2", MB_OK | MB_ICONINFORMATION);
    return 0;
}
