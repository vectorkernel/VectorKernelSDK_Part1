#pragma warning(disable : 28251)
#pragma warning(disable : 28159)

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN

#include <glad/glad.h>
#undef APIENTRY
#include <windows.h>
#include <windowsx.h>
#include <cstdio>
#include <iostream>

#include "Application.h"

#include "Win32Console.h"
#include "Win32MessagePump.h"
#include "Win32WglContext.h"
#include "FrameTimer.h"
#include "Win32AppWindow.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, PSTR, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

#pragma comment(linker, "/subsystem:windows")
#pragma comment(lib, "opengl32.lib")

Application* gApplication = nullptr;
GLuint gVertexArrayObject = 0;
static WglContext gWgl;
static bool gTrackingMouseLeave = false;

static void RenderOneFrame(HWND hwnd)
{
    const ClientSize clientSize = Win32AppWindow::GetClientSize(hwnd);
    glViewport(0, 0, clientSize.width, clientSize.height);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glPointSize(5.0f);
    glBindVertexArray(gVertexArrayObject);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (gApplication)
        gApplication->Render(clientSize);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, PSTR /*szCmdLine*/, int /*iCmdShow*/)
{
    Win32Console::AttachDebugConsole("Example0009_Crosshairs - Logs");

    gApplication = new Application();

    WindowCreateParams windowParams{};
    windowParams.className = L"Example0009_CrosshairsWindow";
    windowParams.title = L"Example0009_Crosshairs";
    windowParams.clientSize = ClientSize{ 1280, 800 };
    windowParams.style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
    windowParams.exStyle = 0;

    Win32AppWindow::RegisterWindowClass(hInstance, WndProc, windowParams.className);
    HWND hwnd = Win32AppWindow::CreateCenteredWindow(hInstance, windowParams);

    if (!gWgl.Create(hwnd, 4, 6, true))
        std::cout << "Failed to create OpenGL context\n";

    gWgl.LoadFunctions();
    gWgl.SetVSync(1);

    glGenVertexArrays(1, &gVertexArrayObject);
    glBindVertexArray(gVertexArrayObject);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    gApplication->Initialize();
    gApplication->OnResize(Win32AppWindow::GetClientSize(hwnd));

    FrameTimer timer;
    timer.Reset();

    MSG msg{};

    while (true)
    {
        if (!Win32MessagePump::PumpOnce(msg))
            break;

        const float deltaTime = timer.TickSeconds();

        if (gApplication)
            gApplication->Update(deltaTime);

        if (gApplication)
            RenderOneFrame(hwnd);

        if (gApplication)
        {
            gWgl.SwapBuffersNow();
            if (gWgl.swapInterval != 0)
                glFinish();
        }
    }

    if (gApplication)
    {
        std::cout << "Shutting down application\n";
        gApplication->Shutdown();
        delete gApplication;
        gApplication = nullptr;
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg)
    {
    case WM_SIZE:
        if (gApplication)
            gApplication->OnResize(Win32AppWindow::GetClientSize(hwnd));
        return 0;

    case WM_RBUTTONDOWN:
        SetCapture(hwnd);
        if (gApplication)
            gApplication->OnRMouseButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_RBUTTONUP:
        ReleaseCapture();
        if (gApplication)
            gApplication->OnRMouseButtonUp();
        return 0;

    case WM_MOUSEMOVE:
        if (!gTrackingMouseLeave)
        {
            TRACKMOUSEEVENT tme{};
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            gTrackingMouseLeave = true;
        }

        if (gApplication)
        {
            const bool rightDown = (wParam & MK_RBUTTON) != 0;
            gApplication->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), rightDown);
        }
        return 0;

    case WM_MOUSELEAVE:
        gTrackingMouseLeave = false;
        if (gApplication)
            gApplication->OnMouseLeave();
        return 0;

    case WM_MOUSEWHEEL:
        if (gApplication)
        {
            POINT p{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &p);
            gApplication->OnMouseWheel(p.x, p.y, GET_WHEEL_DELTA_WPARAM(wParam));
        }
        return 0;

    case WM_KEYDOWN:
        if (gApplication)
        {
            gApplication->OnKeyDown(static_cast<unsigned int>(wParam));
            return 0;
        }
        break;

    case WM_CLOSE:
        if (gApplication)
        {
            gApplication->Shutdown();
            delete gApplication;
            gApplication = nullptr;
            DestroyWindow(hwnd);
        }
        else
        {
            std::cout << "Already shut down!\n";
        }
        return 0;

    case WM_DESTROY:
        if (gVertexArrayObject != 0)
        {
            glBindVertexArray(0);
            glDeleteVertexArrays(1, &gVertexArrayObject);
            gVertexArrayObject = 0;

            gWgl.Destroy(hwnd);
            PostQuitMessage(0);
        }
        else
        {
            std::cout << "Got multiple destroy messages\n";
        }
        return 0;

    case WM_PAINT:
    case WM_ERASEBKGND:
        return 0;
    }

    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
