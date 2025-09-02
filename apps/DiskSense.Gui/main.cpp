#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MessageBoxW(NULL, L"Hello, World!", L"DiskSense.Gui", MB_OK);
    return 0;
}
