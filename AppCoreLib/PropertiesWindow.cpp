#include "pch.h"

#include "PropertiesWindow.h"

#include <commctrl.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "Comctl32.lib")

namespace
{
    constexpr int kFilterComboId = 1000;
    constexpr int kListId = 1001;
    constexpr int kStartXEditId = 1100;
    constexpr int kStartYEditId = 1101;
    constexpr int kStartZEditId = 1102;
    constexpr int kEndXEditId = 1110;
    constexpr int kEndYEditId = 1111;
    constexpr int kEndZEditId = 1112;

    constexpr std::array<int, 3> kStartEditIds{ kStartXEditId, kStartYEditId, kStartZEditId };
    constexpr std::array<int, 3> kEndEditIds{ kEndXEditId, kEndYEditId, kEndZEditId };
    constexpr std::array<const char*, 3> kAxisNames{ "X", "Y", "Z" };

    int InsertColumnA(HWND listView, int index, const LVCOLUMNA& column)
    {
        return static_cast<int>(SendMessageA(listView, LVM_INSERTCOLUMNA, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(&column)));
    }

    int GetItemTextA(HWND listView, int row, int subItem, char* text, int cchTextMax)
    {
        LVITEMA item{};
        item.iSubItem = subItem;
        item.pszText = text;
        item.cchTextMax = cchTextMax;
        return static_cast<int>(SendMessageA(listView, LVM_GETITEMTEXTA, static_cast<WPARAM>(row), reinterpret_cast<LPARAM>(&item)));
    }

    void EnsureCommonControls()
    {
        static bool initialized = false;
        if (initialized)
        {
            return;
        }

        INITCOMMONCONTROLSEX icc{};
        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
        InitCommonControlsEx(&icc);
        initialized = true;
    }

    const char* EntityKindSingularName(EntityKind kind)
    {
        switch (kind)
        {
        case EntityKind::Line: return "Line";
        case EntityKind::Circle: return "Circle";
        case EntityKind::Polyline: return "Polyline";
        default: return "Unknown";
        }
    }

    const char* EntityKindPluralName(EntityKind kind)
    {
        switch (kind)
        {
        case EntityKind::Line: return "Lines";
        case EntityKind::Circle: return "Circles";
        case EntityKind::Polyline: return "Polylines";
        default: return "Unknown";
        }
    }

    std::string CommonOrMixed(const std::vector<const SelectedEntityInfo*>& items, const std::string SelectedEntityInfo::* member)
    {
        if (items.empty())
        {
            return "None";
        }

        const std::string& first = items.front()->*member;
        for (std::size_t i = 1; i < items.size(); ++i)
        {
            if ((items[i]->*member) != first)
            {
                return "Mixed";
            }
        }

        return first;
    }

    std::string TypeLabelForItems(const std::vector<const SelectedEntityInfo*>& items)
    {
        if (items.empty())
        {
            return "None";
        }

        const EntityKind first = items.front()->kind;
        for (std::size_t i = 1; i < items.size(); ++i)
        {
            if (items[i]->kind != first)
            {
                return "Mixed";
            }
        }

        return EntityKindSingularName(first);
    }

    std::string ScopeLabelForFilterKind(PropertiesWindow::SelectionFilterKind filterKind, const std::vector<const SelectedEntityInfo*>& items)
    {
        switch (filterKind)
        {
        case PropertiesWindow::SelectionFilterKind::Line: return "Lines";
        case PropertiesWindow::SelectionFilterKind::Circle: return "Circles";
        case PropertiesWindow::SelectionFilterKind::Polyline: return "Polylines";
        case PropertiesWindow::SelectionFilterKind::All:
        default:
            return items.empty() ? "No Selection" : (TypeLabelForItems(items) == "Mixed" ? "Mixed Selection" : TypeLabelForItems(items) + " Selection");
        }
    }

    std::string FormatCoordinate(float value)
    {
        std::ostringstream ss;
        ss.setf(std::ios::fixed);
        ss.precision(6);
        ss << value;
        return ss.str();
    }

    bool TryParseEditFloat(HWND edit, float& value)
    {
        char buffer[128]{};
        GetWindowTextA(edit, buffer, static_cast<int>(sizeof(buffer)));

        char* end = nullptr;
        const double parsed = std::strtod(buffer, &end);
        if (end == buffer)
        {
            return false;
        }

        while (*end != '\0')
        {
            if (!std::isspace(static_cast<unsigned char>(*end)))
            {
                return false;
            }
            ++end;
        }

        value = static_cast<float>(parsed);
        return true;
    }
}

bool PropertiesWindow::Create(HINSTANCE instance, HWND parent, IPropertiesHost* host, int x, int y, int w, int h)
{
    EnsureCommonControls();

    m_instance = instance;
    m_host = host;

    WNDCLASSA wc{};
    wc.lpfnWndProc = &PropertiesWindow::WndProc;
    wc.hInstance = instance;
    wc.lpszClassName = kWndClass;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassA(&wc);

    m_hwnd = CreateWindowExA(
        WS_EX_TOOLWINDOW,
        kWndClass,
        "Properties",
        WS_OVERLAPPEDWINDOW,
        x, y, w, h,
        parent,
        nullptr,
        instance,
        this);

    if (m_hwnd == nullptr)
    {
        return false;
    }

    Show(false);
    return true;
}

void PropertiesWindow::Destroy()
{
    if (m_hwnd != nullptr)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
        m_filterCombo = nullptr;
        m_list = nullptr;
        m_lineSpaceLabel = nullptr;
        m_startHeader = nullptr;
        m_endHeader = nullptr;
        m_axisLabels = {};
        m_startEdits = {};
        m_endEdits = {};
    }
}

void PropertiesWindow::Show(bool show)
{
    if (m_hwnd == nullptr)
    {
        return;
    }

    ShowWindow(m_hwnd, show ? SW_SHOW : SW_HIDE);
    if (show)
    {
        SetForegroundWindow(m_hwnd);
    }
}

void PropertiesWindow::Toggle()
{
    Show(!IsVisible());
}

bool PropertiesWindow::IsVisible() const
{
    return m_hwnd != nullptr && IsWindowVisible(m_hwnd) != FALSE;
}

void PropertiesWindow::RefreshFromHost()
{
    if (m_host == nullptr || m_list == nullptr || m_filterCombo == nullptr)
    {
        return;
    }

    m_selection = m_host->BuildSelectedEntities();
    SingleLineSelectionInfo lineInfo{};
    if (m_host->TryGetSingleSelectedLine(lineInfo))
    {
        m_singleLineInfo = lineInfo;
    }
    else
    {
        m_singleLineInfo.reset();
    }

    RebuildFilterCombo();
    RefreshPropertyRows();
    RefreshLineEditors();
}

LRESULT CALLBACK PropertiesWindow::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PropertiesWindow* self = nullptr;

    if (message == WM_NCCREATE)
    {
        auto* createStruct = reinterpret_cast<CREATESTRUCTA*>(lParam);
        self = reinterpret_cast<PropertiesWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    else
    {
        self = reinterpret_cast<PropertiesWindow*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    }

    if (self != nullptr)
    {
        return self->HandleMessage(hwnd, message, wParam, lParam);
    }

    return DefWindowProcA(hwnd, message, wParam, lParam);
}

LRESULT PropertiesWindow::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        CreateChildControls(hwnd);
        RefreshFromHost();
        return 0;

    case WM_SIZE:
        ResizeChildControls(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == kFilterComboId && HIWORD(wParam) == CBN_SELCHANGE)
        {
            const int index = static_cast<int>(SendMessageA(m_filterCombo, CB_GETCURSEL, 0, 0));
            if (index >= 0 && index < static_cast<int>(m_filterEntries.size()))
            {
                m_activeFilter = m_filterEntries[static_cast<std::size_t>(index)].filterKind;
                RefreshPropertyRows();
            }
            return 0;
        }

        if (!m_ignoreEditNotifications && HIWORD(wParam) == EN_KILLFOCUS)
        {
            const int controlId = LOWORD(wParam);
            const bool isCoordinateEdit = std::find(kStartEditIds.begin(), kStartEditIds.end(), controlId) != kStartEditIds.end()
                || std::find(kEndEditIds.begin(), kEndEditIds.end(), controlId) != kEndEditIds.end();
            if (isCoordinateEdit)
            {
                ApplyLineEditValues();
                return 0;
            }
        }
        break;

    case WM_NOTIFY:
    {
        const auto* nm = reinterpret_cast<const NMHDR*>(lParam);
        if (nm->hwndFrom == m_list && nm->code == NM_DBLCLK)
        {
            ShowLayerPopupForCurrentRow();
            return 0;
        }
        break;
    }

    case WM_CLOSE:
        Show(false);
        return 0;
    }

    return DefWindowProcA(hwnd, message, wParam, lParam);
}

void PropertiesWindow::CreateChildControls(HWND hwnd)
{
    const HFONT uiFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

    m_filterCombo = CreateWindowExA(
        0,
        WC_COMBOBOXA,
        "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
        0, 0, 100, 200,
        hwnd,
        static_cast<HMENU>(reinterpret_cast<void*>(static_cast<INT_PTR>(kFilterComboId))),
        m_instance,
        nullptr);

    m_lineSpaceLabel = CreateWindowExA(0, "STATIC", "Line Coordinates", WS_CHILD | WS_VISIBLE,
        0, 0, 100, 18, hwnd, nullptr, m_instance, nullptr);
    m_startHeader = CreateWindowExA(0, "STATIC", "Start", WS_CHILD | WS_VISIBLE,
        0, 0, 100, 18, hwnd, nullptr, m_instance, nullptr);
    m_endHeader = CreateWindowExA(0, "STATIC", "End", WS_CHILD | WS_VISIBLE,
        0, 0, 100, 18, hwnd, nullptr, m_instance, nullptr);

    for (int i = 0; i < 3; ++i)
    {
        m_axisLabels[static_cast<std::size_t>(i)] = CreateWindowExA(0, "STATIC", kAxisNames[static_cast<std::size_t>(i)], WS_CHILD | WS_VISIBLE,
            0, 0, 20, 20, hwnd, nullptr, m_instance, nullptr);
        m_startEdits[static_cast<std::size_t>(i)] = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            0, 0, 80, 22, hwnd, static_cast<HMENU>(reinterpret_cast<void*>(static_cast<INT_PTR>(kStartEditIds[static_cast<std::size_t>(i)]))), m_instance, nullptr);
        m_endEdits[static_cast<std::size_t>(i)] = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            0, 0, 80, 22, hwnd, static_cast<HMENU>(reinterpret_cast<void*>(static_cast<INT_PTR>(kEndEditIds[static_cast<std::size_t>(i)]))), m_instance, nullptr);
    }

    m_list = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEWA,
        "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
        0, 0, 100, 100,
        hwnd,
        static_cast<HMENU>(reinterpret_cast<void*>(static_cast<INT_PTR>(kListId))),
        m_instance,
        nullptr);

    const std::array<HWND, 12> controls{
        m_filterCombo,
        m_lineSpaceLabel,
        m_startHeader,
        m_endHeader,
        m_axisLabels[0], m_axisLabels[1], m_axisLabels[2],
        m_startEdits[0], m_startEdits[1], m_startEdits[2],
        m_endEdits[0], m_endEdits[1]
    };
    for (HWND control : controls)
    {
        if (control != nullptr)
            SendMessageA(control, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
    }
    SendMessageA(m_endEdits[2], WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
    SendMessageA(m_list, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);

    ListView_SetExtendedListViewStyle(m_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMNA column{};
    column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    column.pszText = const_cast<LPSTR>("Property");
    column.cx = 150;
    column.iSubItem = 0;
    InsertColumnA(m_list, 0, column);

    column.pszText = const_cast<LPSTR>("Value");
    column.cx = 220;
    column.iSubItem = 1;
    InsertColumnA(m_list, 1, column);
}

void PropertiesWindow::ResizeChildControls(int width, int height)
{
    constexpr int pad = 8;
    constexpr int labelHeight = 18;
    constexpr int editHeight = 22;
    constexpr int rowGap = 6;
    constexpr int comboVisibleHeight = 240;
    constexpr int axisWidth = 18;
    constexpr int columnGap = 8;

    const int clientWidth = (width > 2 * pad ? width - 2 * pad : 10);
    MoveWindow(m_filterCombo, pad, pad, clientWidth, comboVisibleHeight, TRUE);

    const int lineBlockTop = pad + 28 + 8;
    MoveWindow(m_lineSpaceLabel, pad, lineBlockTop, clientWidth, labelHeight, TRUE);

    const int axisLeft = pad;
    const int startLeft = axisLeft + axisWidth + 6;
    const int editWidth = std::max(60, (clientWidth - axisWidth - 6 - columnGap) / 2);
    const int endLeft = startLeft + editWidth + columnGap;

    MoveWindow(m_startHeader, startLeft, lineBlockTop + labelHeight + 2, editWidth, labelHeight, TRUE);
    MoveWindow(m_endHeader, endLeft, lineBlockTop + labelHeight + 2, editWidth, labelHeight, TRUE);

    const int firstRowTop = lineBlockTop + labelHeight + 2 + labelHeight + 2;
    for (int i = 0; i < 3; ++i)
    {
        const int rowTop = firstRowTop + i * (editHeight + rowGap);
        MoveWindow(m_axisLabels[static_cast<std::size_t>(i)], axisLeft, rowTop + 3, axisWidth, labelHeight, TRUE);
        MoveWindow(m_startEdits[static_cast<std::size_t>(i)], startLeft, rowTop, editWidth, editHeight, TRUE);
        MoveWindow(m_endEdits[static_cast<std::size_t>(i)], endLeft, rowTop, editWidth, editHeight, TRUE);
    }

    const int listTop = firstRowTop + 3 * (editHeight + rowGap) + 6;
    MoveWindow(m_list, pad, listTop, clientWidth, (height > listTop + pad ? height - listTop - pad : 10), TRUE);
}

void PropertiesWindow::PopulateList(const std::vector<std::pair<std::string, std::string>>& rows)
{
    ListView_DeleteAllItems(m_list);

    int rowIndex = 0;
    for (const auto& row : rows)
    {
        LVITEMA item{};
        item.mask = LVIF_TEXT;
        item.iItem = rowIndex;
        item.iSubItem = 0;
        item.pszText = const_cast<LPSTR>(row.first.c_str());
        SendMessageA(m_list, LVM_INSERTITEMA, 0, reinterpret_cast<LPARAM>(&item));

        LVITEMA value{};
        value.iSubItem = 1;
        value.pszText = const_cast<LPSTR>(row.second.c_str());
        SendMessageA(m_list, LVM_SETITEMTEXTA, static_cast<WPARAM>(rowIndex), reinterpret_cast<LPARAM>(&value));
        ++rowIndex;
    }
}

void PropertiesWindow::RebuildFilterCombo()
{
    std::map<EntityKind, int> counts;
    for (const SelectedEntityInfo& item : m_selection)
    {
        counts[item.kind] += 1;
    }

    m_filterEntries.clear();

    const int total = static_cast<int>(m_selection.size());
    const int representedTypeCount = static_cast<int>(counts.size());

    SelectionFilterEntry topEntry{};
    topEntry.filterKind = SelectionFilterKind::All;
    topEntry.count = total;

    if (total == 0)
    {
        topEntry.entityKind = EntityKind::Unknown;
        topEntry.label = "No Selection";
    }
    else if (representedTypeCount > 1)
    {
        topEntry.entityKind = EntityKind::Unknown;
        topEntry.label = "Mixed Selection (" + std::to_string(total) + ")";
    }
    else
    {
        topEntry.entityKind = counts.begin()->first;
        topEntry.label = std::string(EntityKindPluralName(topEntry.entityKind)) + " (" + std::to_string(total) + ")";
    }

    m_filterEntries.push_back(topEntry);

    auto addFilter = [this, &counts](EntityKind kind, SelectionFilterKind filterKind)
    {
        const auto it = counts.find(kind);
        if (it == counts.end() || it->second <= 0)
        {
            return;
        }

        SelectionFilterEntry entry{};
        entry.filterKind = filterKind;
        entry.entityKind = kind;
        entry.count = it->second;
        entry.label = std::string(EntityKindPluralName(kind)) + " (" + std::to_string(entry.count) + ")";
        m_filterEntries.push_back(entry);
    };

    if (representedTypeCount > 1)
    {
        addFilter(EntityKind::Line, SelectionFilterKind::Line);
        addFilter(EntityKind::Circle, SelectionFilterKind::Circle);
        addFilter(EntityKind::Polyline, SelectionFilterKind::Polyline);
    }
    else
    {
        m_activeFilter = SelectionFilterKind::All;
    }

    const bool activeStillPresent = std::any_of(m_filterEntries.begin(), m_filterEntries.end(), [this](const SelectionFilterEntry& entry)
    {
        return entry.filterKind == m_activeFilter;
    });

    if (!activeStillPresent)
    {
        m_activeFilter = SelectionFilterKind::All;
    }

    SendMessageA(m_filterCombo, CB_RESETCONTENT, 0, 0);

    int activeIndex = 0;
    for (int i = 0; i < static_cast<int>(m_filterEntries.size()); ++i)
    {
        const auto& entry = m_filterEntries[static_cast<std::size_t>(i)];
        const int idx = static_cast<int>(SendMessageA(m_filterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(entry.label.c_str())));
        if (entry.filterKind == m_activeFilter)
        {
            activeIndex = idx;
        }
    }

    SendMessageA(m_filterCombo, CB_SETCURSEL, activeIndex, 0);
}

void PropertiesWindow::RefreshPropertyRows()
{
    const std::vector<const SelectedEntityInfo*> items = GetFilteredSelection();

    std::vector<std::pair<std::string, std::string>> rows;
    rows.emplace_back("Selection Scope", ScopeLabelForFilterKind(m_activeFilter, items));
    rows.emplace_back("Selection Count", std::to_string(items.size()));
    rows.emplace_back("Names", items.empty() ? std::string("None") : CommonOrMixed(items, &SelectedEntityInfo::displayName));
    rows.emplace_back("Type", TypeLabelForItems(items));
    rows.emplace_back("Layer", CommonOrMixed(items, &SelectedEntityInfo::layerName));
    rows.emplace_back("Color", CommonOrMixed(items, &SelectedEntityInfo::colorName));
    rows.emplace_back("Visible", items.empty() ? std::string("n/a") : std::string("Demo only"));

    PopulateList(rows);
}

void PropertiesWindow::RefreshLineEditors()
{
    const bool enabled = m_singleLineInfo.has_value();
    const std::string spaceLabel = enabled
        ? std::string("Line Coordinates (") + (m_singleLineInfo->screenSpace ? "screen space" : "model/paper space") + ")"
        : "Line Coordinates (single line selection required)";
    SetWindowTextA(m_lineSpaceLabel, spaceLabel.c_str());

    m_ignoreEditNotifications = true;
    for (int i = 0; i < 3; ++i)
    {
        EnableWindow(m_startEdits[static_cast<std::size_t>(i)], enabled ? TRUE : FALSE);
        EnableWindow(m_endEdits[static_cast<std::size_t>(i)], enabled ? TRUE : FALSE);

        if (enabled)
        {
            const float startValue = (i == 0) ? m_singleLineInfo->startX : (i == 1) ? m_singleLineInfo->startY : m_singleLineInfo->startZ;
            const float endValue = (i == 0) ? m_singleLineInfo->endX : (i == 1) ? m_singleLineInfo->endY : m_singleLineInfo->endZ;
            const std::string startText = FormatCoordinate(startValue);
            const std::string endText = FormatCoordinate(endValue);
            SetWindowTextA(m_startEdits[static_cast<std::size_t>(i)], startText.c_str());
            SetWindowTextA(m_endEdits[static_cast<std::size_t>(i)], endText.c_str());
        }
        else
        {
            SetWindowTextA(m_startEdits[static_cast<std::size_t>(i)], "");
            SetWindowTextA(m_endEdits[static_cast<std::size_t>(i)], "");
        }
    }
    m_ignoreEditNotifications = false;
}

bool PropertiesWindow::ApplyLineEditValues()
{
    if (!m_host || !m_singleLineInfo.has_value())
    {
        return false;
    }

    SingleLineSelectionInfo updated = *m_singleLineInfo;
    if (!TryParseEditFloat(m_startEdits[0], updated.startX)
        || !TryParseEditFloat(m_startEdits[1], updated.startY)
        || !TryParseEditFloat(m_startEdits[2], updated.startZ)
        || !TryParseEditFloat(m_endEdits[0], updated.endX)
        || !TryParseEditFloat(m_endEdits[1], updated.endY)
        || !TryParseEditFloat(m_endEdits[2], updated.endZ))
    {
        MessageBeep(MB_ICONWARNING);
        RefreshLineEditors();
        return false;
    }

    if (!m_host->UpdateSingleSelectedLine(updated))
    {
        MessageBeep(MB_ICONWARNING);
        RefreshFromHost();
        return false;
    }

    m_singleLineInfo = updated;
    RefreshFromHost();
    return true;
}

std::vector<const SelectedEntityInfo*> PropertiesWindow::GetFilteredSelection() const
{
    std::vector<const SelectedEntityInfo*> filtered;
    filtered.reserve(m_selection.size());

    const EntityKind entityFilter = ActiveEntityFilterKind();
    for (const SelectedEntityInfo& item : m_selection)
    {
        if (entityFilter == EntityKind::Unknown || item.kind == entityFilter)
        {
            filtered.push_back(&item);
        }
    }

    return filtered;
}

void PropertiesWindow::ShowLayerPopupForCurrentRow()
{
    if (m_host == nullptr)
    {
        return;
    }

    const int row = ListView_GetNextItem(m_list, -1, LVNI_SELECTED);
    if (row < 0)
    {
        return;
    }

    char propertyName[256]{};
    GetItemTextA(m_list, row, 0, propertyName, static_cast<int>(sizeof(propertyName)) - 1);
    if (std::string(propertyName) != "Layer")
    {
        return;
    }

    const std::vector<std::pair<std::string, std::uint32_t>> choices = m_host->BuildLayerChoices();
    if (choices.empty())
    {
        return;
    }

    HMENU menu = CreatePopupMenu();
    constexpr UINT kBaseCommand = 30000;

    for (UINT i = 0; i < choices.size(); ++i)
    {
        AppendMenuA(menu, MF_STRING, kBaseCommand + i, choices[i].first.c_str());
    }

    RECT rect{};
    ListView_GetItemRect(m_list, row, &rect, LVIR_BOUNDS);
    POINT pt{ rect.left, rect.bottom };
    ClientToScreen(m_list, &pt);

    const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, m_hwnd, nullptr);
    DestroyMenu(menu);

    if (command >= kBaseCommand && command < kBaseCommand + choices.size())
    {
        const std::size_t index = static_cast<std::size_t>(command - kBaseCommand);
        if (m_host->AssignSelectionLayer(choices[index].second, ActiveEntityFilterKind()))
        {
            RefreshFromHost();
        }
    }
}

EntityKind PropertiesWindow::ActiveEntityFilterKind() const
{
    switch (m_activeFilter)
    {
    case SelectionFilterKind::Line: return EntityKind::Line;
    case SelectionFilterKind::Circle: return EntityKind::Circle;
    case SelectionFilterKind::Polyline: return EntityKind::Polyline;
    case SelectionFilterKind::All:
    default: return EntityKind::Unknown;
    }
}
