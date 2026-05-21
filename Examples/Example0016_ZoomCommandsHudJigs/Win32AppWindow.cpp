#include "Win32AppWindow.h"

ATOM Win32AppWindow::RegisterWindowClass(HINSTANCE instance, WNDPROC wndProc, LPCWSTR className)
{
    WNDCLASSEX wndclass{};
    wndclass.cbSize = sizeof(WNDCLASSEX);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = wndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = instance;
    wndclass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wndclass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndclass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wndclass.lpszMenuName = nullptr;
    wndclass.lpszClassName = className;
    return RegisterClassEx(&wndclass);
}

HWND Win32AppWindow::CreateCenteredWindow(HINSTANCE instance, const WindowCreateParams& params)
{
    const RECT windowRect = BuildCenteredWindowRect(params.clientSize, params.style, params.exStyle);

    return CreateWindowEx(
        params.exStyle,
        params.className,
        params.title,
        params.style,
        windowRect.left,
        windowRect.top,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        instance,
        nullptr);
}

ClientSize Win32AppWindow::GetClientSize(HWND hwnd)
{
    RECT clientRect{};
    GetClientRect(hwnd, &clientRect);

    return ClientSize{
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top
    };
}

RECT Win32AppWindow::BuildCenteredWindowRect(const ClientSize& clientSize, DWORD style, DWORD exStyle)
{
    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    RECT windowRect{};
    SetRect(
        &windowRect,
        (screenWidth / 2) - (clientSize.width / 2),
        (screenHeight / 2) - (clientSize.height / 2),
        (screenWidth / 2) + (clientSize.width / 2),
        (screenHeight / 2) + (clientSize.height / 2));

    AdjustWindowRectEx(&windowRect, style, FALSE, exStyle);
    return windowRect;
}
