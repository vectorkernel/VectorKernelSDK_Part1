#pragma once

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

#include "WindowTypes.h"

struct WindowCreateParams
{
    LPCWSTR className = L"Win32 OpenGL Window";
    LPCWSTR title = L"Example001 OpenGL App";
    ClientSize clientSize{ 800, 600 };
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    DWORD exStyle = 0;
};

class Win32AppWindow
{
public:
    static ATOM RegisterWindowClass(HINSTANCE instance, WNDPROC wndProc, LPCWSTR className);
    static HWND CreateCenteredWindow(HINSTANCE instance, const WindowCreateParams& params);
    static ClientSize GetClientSize(HWND hwnd);

private:
    static RECT BuildCenteredWindowRect(const ClientSize& clientSize, DWORD style, DWORD exStyle);
};
