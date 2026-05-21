#pragma once

#include <functional>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "..\RenderCoreLib\hersheyfont.h"

namespace AppCore
{
struct UiVertex
{
    glm::vec3 pos{};
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
};

class IHersheyCommandHost
{
public:
    virtual ~IHersheyCommandHost() = default;
    virtual void OnHersheyCommandSubmitted(const std::string& commandText) = 0;
    virtual void RequestHersheyCommandRedraw() = 0;
};

class HersheyCommandWindow
{
public:
    struct State
    {
        bool visible = true;
        bool focused = true;
        bool hovered = false;
        bool dragging = false;
        bool draggingScrollbar = false;
        bool selecting = false;
        bool resizing = false;
        bool minimized = false;
        bool dockToBottom = true;
        bool hasFocus = true;
        bool hasMouseFocus = false;

        glm::vec2 origin { 0.0f, 0.0f };
        glm::vec2 size { 470.0f, 244.0f };
        glm::vec2 dragOffset { 0.0f, 0.0f };
        glm::vec2 resizeStartMouse { 0.0f, 0.0f };
        glm::vec2 resizeStartSize { 0.0f, 0.0f };

        float titleBarHeight = 24.0f;
        float scrollbarWidth = 14.0f;
        float resizeHandleSize = 18.0f;
        float padding = 8.0f;
        float buttonSize = 16.0f;
        float textScale = 0.34f;
        float textStroke = 1.0f;
        int backscrollLimit = 256;
        int scrollOffsetLines = 0;
        int expandedHeightPx = 210;
        int minimumHeightPx = 140;

        int selectionStart = -1;
        int selectionEnd = -1;

        std::string inputBuffer;
        std::vector<std::string> history;
        std::vector<glm::vec4> backgroundPalette {
            glm::vec4(0.08f, 0.02f, 0.02f, 0.72f),
            glm::vec4(0.02f, 0.05f, 0.10f, 0.62f),
            glm::vec4(0.05f, 0.05f, 0.05f, 0.78f),
            glm::vec4(0.10f, 0.08f, 0.02f, 0.60f)
        };
        int backgroundIndex = 0;
    };

    explicit HersheyCommandWindow(IHersheyCommandHost& host);

    void SetFont(hershey_font* font);
    void SetViewportSize(int width, int height);
    void Update(float deltaTime);

    void SetPromptLineProvider(std::function<std::string()> provider);
    void SetStatusLineProvider(std::function<std::string()> provider);
    void SetTitle(std::string title);

    bool HandleMouseMove(int x, int y);
    void HandleMouseLeave();
    bool HandleLeftButtonDown(int x, int y);
    void HandleLeftButtonUp();
    bool HandleMouseWheel(int x, int y, int wheelDelta);
    bool HandleKeyDown(unsigned int vk, bool ctrlHeld, bool shiftHeld);
    bool HandleChar(unsigned int ch);

    bool WantsTextCursor(int x, int y) const;
    bool IsVisible() const;
    bool IsHovered() const;
    bool IsFocused() const;
    void SetVisible(bool visible);
    void ToggleVisible();
    void SetFocused(bool focused);

    void AddHistory(const std::string& line);
    void ClearHistory();
    void AdvanceBackgroundPalette();
    void SetInputBuffer(std::string text);
    const std::string& GetInputBuffer() const;
    State& GetState();
    const State& GetState() const;

    void BuildOverlay(std::vector<UiVertex>& outLines, std::vector<UiVertex>& outTriangles) const;

private:
    float CommandTextLineHeightPx() const;
    float CommandInputBandHeightPx() const;
    int VisibleCommandLineCapacity() const;
    int TotalCommandLineCount() const;
    std::string GetPromptLine() const;
    std::vector<std::string> GetVisibleCommandLines() const;
    void ClampWindow();
    bool IsPointInWindow(float x, float y) const;
    bool IsPointInTitleBar(float x, float y) const;
    bool IsPointInScrollbar(float x, float y) const;
    bool IsPointInTextArea(float x, float y) const;
    bool IsPointInMinimizeButton(float x, float y) const;
    bool IsPointInCloseButton(float x, float y) const;
    int HitTestTextLine(float x, float y) const;
    std::string BuildSelectedText() const;
    std::string BuildAllText() const;
    void CopyTextToClipboard(const std::string& text) const;
    void ScrollBy(int deltaLines);

    void AddFilledRect(float x0, float y0, float x1, float y1, const glm::vec4& color, std::vector<UiVertex>& outTriangles) const;
    void AddLinePx(float x0, float y0, float x1, float y1, const glm::vec4& color, std::vector<UiVertex>& outLines) const;
    void AppendTextBlock(const std::string& text,
                         float x,
                         float y,
                         const glm::vec4& color,
                         float boxWidth,
                         float boxHeight,
                         bool wordWrap,
                         std::vector<UiVertex>& outLines) const;

    IHersheyCommandHost& m_host;
    State m_state;
    hershey_font* m_font = nullptr;
    int m_clientWidth = 1;
    int m_clientHeight = 1;
    float m_elapsedSeconds = 0.0f;
    std::function<std::string()> m_promptLineProvider;
    std::function<std::string()> m_statusLineProvider;
    std::string m_title = "Hershey Command Window";
};
}
