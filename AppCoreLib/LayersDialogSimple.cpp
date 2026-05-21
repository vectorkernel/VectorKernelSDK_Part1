#include "pch.h"

#include "LayersDialogSimple.h"

#include <cstdio>
#include <string>
#include <commctrl.h>
#pragma comment(lib, "Comctl32.lib")

namespace
{
    void EnsureCommonControls()
    {
        static bool initialized = false;
        if (initialized)
            return;

        INITCOMMONCONTROLSEX icc{};
        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_LISTVIEW_CLASSES;
        InitCommonControlsEx(&icc);
        initialized = true;
    }

    void SetItemTextA(HWND listView, int row, int subItem, LPSTR text)
    {
        LVITEMA item{};
        item.iSubItem = subItem;
        item.pszText = text;
        SendMessageA(listView, LVM_SETITEMTEXTA, static_cast<WPARAM>(row), reinterpret_cast<LPARAM>(&item));
    }

    int ToByte(float value)
    {
        const float scaled = value * 255.0f;
        if (scaled <= 0.0f)
            return 0;
        if (scaled >= 255.0f)
            return 255;
        return static_cast<int>(scaled + 0.5f);
    }

    COLORREF ToCOLORREF(const glm::vec4& c)
    {
        return RGB(ToByte(c.r), ToByte(c.g), ToByte(c.b));
    }
}

bool LayersDialogSimple::Create(HINSTANCE instance, HWND parent, const LayerTable* table, int x, int y, int w, int h)
{
    EnsureCommonControls();

    m_instance = instance;
    m_table = table;

    WNDCLASSA wc{};
    wc.lpfnWndProc = &LayersDialogSimple::WndProc;
    wc.hInstance = instance;
    wc.lpszClassName = kWndClass;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassA(&wc);

    m_hwnd = CreateWindowExA(
        WS_EX_TOOLWINDOW,
        kWndClass,
        "Layers",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
        x, y, w, h,
        parent,
        nullptr,
        instance,
        this);

    if (m_hwnd == nullptr)
        return false;

    Show(false);
    return true;
}

void LayersDialogSimple::Destroy()
{
    if (m_hwnd != nullptr)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
        m_list = nullptr;
    }
}

void LayersDialogSimple::SetLayerTable(const LayerTable* table)
{
    m_table = table;
}

void LayersDialogSimple::Show(bool show)
{
    if (m_hwnd == nullptr)
        return;

    ShowWindow(m_hwnd, show ? SW_SHOW : SW_HIDE);
    if (show)
    {
        SetForegroundWindow(m_hwnd);
        Refresh();
    }
}

void LayersDialogSimple::Toggle()
{
    Show(!IsVisible());
}

bool LayersDialogSimple::IsVisible() const
{
    return m_hwnd != nullptr && IsWindowVisible(m_hwnd) != FALSE;
}

void LayersDialogSimple::Refresh()
{
    if (m_list == nullptr)
        return;

    PopulateList();
}

LRESULT CALLBACK LayersDialogSimple::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LayersDialogSimple* self = nullptr;
    if (message == WM_NCCREATE)
    {
        auto* createStruct = reinterpret_cast<CREATESTRUCTA*>(lParam);
        self = reinterpret_cast<LayersDialogSimple*>(createStruct->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    else
    {
        self = reinterpret_cast<LayersDialogSimple*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    }

    if (self != nullptr)
        return self->HandleMessage(hwnd, message, wParam, lParam);

    return DefWindowProcA(hwnd, message, wParam, lParam);
}

LRESULT LayersDialogSimple::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        CreateChildControls(hwnd);
        Refresh();
        return 0;

    case WM_SIZE:
        ResizeChildControls(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_NOTIFY:
    {
        const auto* nm = reinterpret_cast<const NMHDR*>(lParam);
        if (nm->hwndFrom == m_list && nm->code == NM_CUSTOMDRAW)
        {
            const auto* custom = reinterpret_cast<const NMLVCUSTOMDRAW*>(lParam);
            switch (custom->nmcd.dwDrawStage)
            {
            case CDDS_PREPAINT:
                return CDRF_NOTIFYITEMDRAW;
            case CDDS_ITEMPREPAINT:
                return CDRF_NOTIFYSUBITEMDRAW;
            case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
                if (custom->iSubItem == kColSwatch)
                {
                    DrawSwatch(custom);
                    return CDRF_SKIPDEFAULT;
                }
                return CDRF_DODEFAULT;
            default:
                break;
            }
        }

        if (nm->hwndFrom == m_list && nm->code == NM_DBLCLK && m_table != nullptr)
        {
            const auto* activate = reinterpret_cast<const NMITEMACTIVATE*>(lParam);
            if (activate->iItem >= 0)
            {
                LayerTable* table = const_cast<LayerTable*>(m_table);
                const auto& layers = table->GetLayers();
                const std::size_t row = static_cast<std::size_t>(activate->iItem);
                if (row < layers.size())
                {
                    const std::uint32_t layerId = layers[row].id;
                    bool changed = false;

                    if (activate->iSubItem == kColOn)
                        changed = table->SetLayerOn(layerId, !layers[row].on);
                    else if (activate->iSubItem == kColLocked)
                        changed = table->SetLayerLocked(layerId, !layers[row].locked);
                    else if (activate->iSubItem == kColName || activate->iSubItem == kColCurrent || activate->iSubItem == kColId)
                    {
                        table->SetCurrentLayer(layerId);
                        changed = (table->CurrentLayerId() == layerId);
                    }

                    if (changed)
                    {
                        Refresh();
                        HWND parent = GetParent(hwnd);
                        if (parent != nullptr)
                            PostMessageA(parent, kMsgLayerStateChanged, 0, 0);
                    }
                }
            }
            return 0;
        }

        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == 2002 && HIWORD(wParam) == BN_CLICKED)
        {
            SetCurrentFromSelectedRow();
            return 0;
        }
        break;

    case WM_CLOSE:
        Show(false);
        return 0;
    }

    return DefWindowProcA(hwnd, message, wParam, lParam);
}

void LayersDialogSimple::CreateChildControls(HWND hwnd)
{
    m_list = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEWA,
        "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
        0, 0, 100, 100,
        hwnd,
        reinterpret_cast<HMENU>(2001),
        m_instance,
        nullptr);

    m_btnSetCurrent = CreateWindowExA(
        0,
        "BUTTON",
        "Set Current",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 100, 26,
        hwnd,
        reinterpret_cast<HMENU>(2002),
        m_instance,
        nullptr);

    ListView_SetExtendedListViewStyle(m_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    auto addColumn = [&](int index, int width, const char* title)
    {
            LVCOLUMNA col{};
            col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            col.fmt = LVCFMT_LEFT;
            col.cx = width;
            col.pszText = const_cast<LPSTR>(title);
            col.iSubItem = index;
            SendMessageA(m_list, LVM_INSERTCOLUMNA, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(&col));
    };

    addColumn(kColName, 180, "Name");
    addColumn(kColCurrent, 80, "Current");
    addColumn(kColOn, 60, "On");
    addColumn(kColLocked, 80, "Locked");
    addColumn(kColColorHex, 110, "Color");
    addColumn(kColSwatch, 70, "Swatch");
    addColumn(kColLinetype, 110, "Linetype");
    addColumn(kColId, 80, "Id");
}

void LayersDialogSimple::ResizeChildControls(int width, int height)
{
    if (m_list == nullptr)
        return;

    const int buttonHeight = 28;
    const int margin = 8;
    MoveWindow(m_list, margin, margin, width - (margin * 2), height - (margin * 3) - buttonHeight, TRUE);
    if (m_btnSetCurrent != nullptr)
        MoveWindow(m_btnSetCurrent, width - margin - 110, height - margin - buttonHeight, 110, buttonHeight, TRUE);
}

void LayersDialogSimple::PopulateList()
{
    ListView_DeleteAllItems(m_list);
    if (m_table == nullptr)
        return;

    int row = 0;
    char buffer[128]{};

    for (const LayerTable::Layer& layer : m_table->GetLayers())
    {
        LVITEMA item{};
        item.mask = LVIF_TEXT;
        item.iItem = row;
        item.iSubItem = kColName;
        item.pszText = const_cast<LPSTR>(layer.name.c_str());

        SendMessageA(
            m_list,
            LVM_INSERTITEMA,
            0,
            reinterpret_cast<LPARAM>(&item));

        std::snprintf(buffer, sizeof(buffer), "%s", layer.id == m_table->CurrentLayerId() ? "Yes" : "" );
        SetItemTextA(m_list, row, kColCurrent, buffer);

        std::snprintf(buffer, sizeof(buffer), "%s", layer.on ? "Yes" : "No");
        SetItemTextA(m_list, row, kColOn, buffer);

        std::snprintf(buffer, sizeof(buffer), "%s", layer.locked ? "Yes" : "No");
        SetItemTextA(m_list, row, kColLocked, buffer);

        std::snprintf(
            buffer,
            sizeof(buffer),
            "#%02X%02X%02X",
            ToByte(layer.defaultColor.r),
            ToByte(layer.defaultColor.g),
            ToByte(layer.defaultColor.b));
        SetItemTextA(m_list, row, kColColorHex, buffer);

        SetItemTextA(m_list, row, kColSwatch, const_cast<LPSTR>(""));
        SetItemTextA(m_list, row, kColLinetype, const_cast<LPSTR>(layer.defaultLinetype.c_str()));

        std::snprintf(buffer, sizeof(buffer), "%u", layer.id);
        SetItemTextA(m_list, row, kColId, buffer);

        ++row;
    }
}

int LayersDialogSimple::GetSelectedRow() const
{
    if (m_list == nullptr)
        return -1;

    return ListView_GetNextItem(m_list, -1, LVNI_SELECTED);
}

void LayersDialogSimple::SetCurrentFromSelectedRow()
{
    if (m_table == nullptr)
        return;

    const int row = GetSelectedRow();
    if (row < 0)
        return;

    LayerTable* table = const_cast<LayerTable*>(m_table);
    const auto& layers = table->GetLayers();
    const std::size_t index = static_cast<std::size_t>(row);
    if (index >= layers.size())
        return;

    table->SetCurrentLayer(layers[index].id);
    Refresh();

    HWND parent = (m_hwnd != nullptr) ? GetParent(m_hwnd) : nullptr;
    if (parent != nullptr)
        PostMessageA(parent, kMsgLayerStateChanged, 0, 0);
}

RECT LayersDialogSimple::GetSubItemRect(int row, int subItem) const
{
    RECT rect{};
    rect.top = subItem;
    rect.left = LVIR_BOUNDS;
    SendMessageA(m_list, LVM_GETSUBITEMRECT, static_cast<WPARAM>(row), reinterpret_cast<LPARAM>(&rect));
    return rect;
}

void LayersDialogSimple::DrawSwatch(const NMLVCUSTOMDRAW* customDraw) const
{
    if (m_table == nullptr)
        return;

    const int row = static_cast<int>(customDraw->nmcd.dwItemSpec);
    const auto& layers = m_table->GetLayers();
    if (row < 0 || row >= static_cast<int>(layers.size()))
        return;

    RECT rect = GetSubItemRect(row, kColSwatch);
    InflateRect(&rect, -6, -4);

    HDC dc = customDraw->nmcd.hdc;
    HBRUSH brush = CreateSolidBrush(ToCOLORREF(layers[static_cast<std::size_t>(row)].defaultColor));
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
    FrameRect(dc, &rect, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
}
