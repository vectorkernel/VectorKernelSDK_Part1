#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "DialogContracts.h"

#include <array>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class PropertiesWindow
{
public:
    static constexpr const char* kWndClass = "AppCorePropertiesWindow";

    bool Create(HINSTANCE instance, HWND parent, IPropertiesHost* host, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT, int w = 420, int h = 420);
    void Destroy();

    void Show(bool show);
    void Toggle();
    bool IsVisible() const;

    void RefreshFromHost();

public:
    enum class SelectionFilterKind
    {
        All = 0,
        Line,
        Circle,
        Polyline
    };

private:
    struct SelectionFilterEntry
    {
        SelectionFilterKind filterKind = SelectionFilterKind::All;
        EntityKind entityKind = EntityKind::Unknown;
        std::string label;
        int count = 0;
    };

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void CreateChildControls(HWND hwnd);
    void ResizeChildControls(int width, int height);
    void PopulateList(const std::vector<std::pair<std::string, std::string>>& rows);
    void RebuildFilterCombo();
    void RefreshPropertyRows();
    void RefreshLineEditors();
    bool ApplyLineEditValues();
    std::vector<const SelectedEntityInfo*> GetFilteredSelection() const;
    void ShowLayerPopupForCurrentRow();
    EntityKind ActiveEntityFilterKind() const;

private:
    HINSTANCE m_instance = nullptr;
    HWND m_hwnd = nullptr;
    HWND m_filterCombo = nullptr;
    HWND m_list = nullptr;
    HWND m_lineSpaceLabel = nullptr;
    HWND m_startHeader = nullptr;
    HWND m_endHeader = nullptr;
    std::array<HWND, 3> m_axisLabels{};
    std::array<HWND, 3> m_startEdits{};
    std::array<HWND, 3> m_endEdits{};
    IPropertiesHost* m_host = nullptr;

    std::vector<SelectedEntityInfo> m_selection;
    std::optional<SingleLineSelectionInfo> m_singleLineInfo;
    bool m_ignoreEditNotifications = false;
    std::vector<SelectionFilterEntry> m_filterEntries;
    SelectionFilterKind m_activeFilter = SelectionFilterKind::All;
};
