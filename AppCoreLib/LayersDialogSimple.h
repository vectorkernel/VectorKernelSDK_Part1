#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>

#include "..\RenderCoreLib\LayerTable.h"

class LayersDialogSimple
{
public:
    static constexpr const char* kWndClass = "AppCoreLayersDialogSimple";
    static constexpr UINT kMsgLayerStateChanged = WM_APP + 7;

    bool Create(HINSTANCE instance, HWND parent, const LayerTable* table, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT, int w = 760, int h = 420);
    void Destroy();

    void SetLayerTable(const LayerTable* table);
    void Show(bool show);
    void Toggle();
    bool IsVisible() const;
    void Refresh();

private:
    static constexpr int kColName = 0;
    static constexpr int kColCurrent = 1;
    static constexpr int kColOn = 2;
    static constexpr int kColLocked = 3;
    static constexpr int kColColorHex = 4;
    static constexpr int kColSwatch = 5;
    static constexpr int kColLinetype = 6;
    static constexpr int kColId = 7;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void CreateChildControls(HWND hwnd);
    void ResizeChildControls(int width, int height);
    void PopulateList();
    int GetSelectedRow() const;
    void SetCurrentFromSelectedRow();
    RECT GetSubItemRect(int row, int subItem) const;
    void DrawSwatch(const NMLVCUSTOMDRAW* customDraw) const;

private:
    HINSTANCE m_instance = nullptr;
    HWND m_hwnd = nullptr;
    HWND m_list = nullptr;
    HWND m_btnSetCurrent = nullptr;
    const LayerTable* m_table = nullptr;
};
