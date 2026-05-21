#include "pch.h"
#include "HersheyCommandWindow.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include <utility>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "..\EntityCoreLib\LineEntity.h"
#include "..\EntityCoreLib\TextEntity.h"
#include "..\RenderCoreLib\HersheyTextBuilder.h"

namespace AppCore
{
HersheyCommandWindow::HersheyCommandWindow(IHersheyCommandHost& host)
    : m_host(host)
{
}

void HersheyCommandWindow::SetFont(hershey_font* font) { m_font = font; }
void HersheyCommandWindow::SetViewportSize(int width, int height) { m_clientWidth = std::max(1, width); m_clientHeight = std::max(1, height); ClampWindow(); }
void HersheyCommandWindow::Update(float deltaTime) { m_elapsedSeconds += deltaTime; }
void HersheyCommandWindow::SetPromptLineProvider(std::function<std::string()> provider) { m_promptLineProvider = std::move(provider); }
void HersheyCommandWindow::SetStatusLineProvider(std::function<std::string()> provider) { m_statusLineProvider = std::move(provider); }
void HersheyCommandWindow::SetTitle(std::string title) { m_title = std::move(title); }

bool HersheyCommandWindow::HandleMouseMove(int x, int y)
{
    m_state.hovered = m_state.visible && IsPointInWindow(static_cast<float>(x), static_cast<float>(y));
    m_state.hasMouseFocus = m_state.hovered;
    if (m_state.dragging)
    {
        m_state.origin = glm::vec2(static_cast<float>(x), static_cast<float>(y)) - m_state.dragOffset;
        ClampWindow();
        m_host.RequestHersheyCommandRedraw();
        return true;
    }
    if (m_state.draggingScrollbar)
    {
        const float headerBottom = m_state.origin.y + m_state.titleBarHeight;
        const float contentBottom = m_state.origin.y + m_state.size.y;
        const float textTop = headerBottom + m_state.padding;
        const float textBottom = contentBottom - m_state.padding - CommandInputBandHeightPx();
        const float trackTop = textTop;
        const float trackBottom = textBottom;
        const int visibleLines = std::max(1, VisibleCommandLineCapacity());
        const int totalLines = std::max(visibleLines, TotalCommandLineCount());
        const float trackHeight = std::max(1.0f, trackBottom - trackTop);
        const float thumbHeight = std::max(20.0f, trackHeight * (static_cast<float>(visibleLines) / static_cast<float>(totalLines)));
        const float maxThumbTravel = std::max(1.0f, trackHeight - thumbHeight);
        const float thumbCenter = static_cast<float>(y) - (thumbHeight * 0.5f);
        const float clampedThumb = std::clamp(thumbCenter, trackTop, trackTop + maxThumbTravel);
        const float scrollT = (clampedThumb - trackTop) / maxThumbTravel;
        const int maxScroll = std::max(0, totalLines - visibleLines);
        m_state.scrollOffsetLines = static_cast<int>(std::round(scrollT * static_cast<float>(maxScroll)));
        m_host.RequestHersheyCommandRedraw();
        return true;
    }
    if (m_state.selecting)
    {
        m_state.selectionEnd = HitTestTextLine(static_cast<float>(x), static_cast<float>(y));
        m_host.RequestHersheyCommandRedraw();
        return true;
    }
    return m_state.hovered;
}

void HersheyCommandWindow::HandleMouseLeave()
{
    m_state.hovered = false; m_state.hasMouseFocus = false; m_state.dragging = false; m_state.draggingScrollbar = false; m_state.selecting = false; m_state.resizing = false;
}

bool HersheyCommandWindow::HandleLeftButtonDown(int x, int y)
{
    if (!m_state.visible) return false;
    const float fx = static_cast<float>(x), fy = static_cast<float>(y);
    if (!IsPointInWindow(fx, fy)) { m_state.focused = false; m_state.hasFocus = false; return false; }
    m_state.focused = true; m_state.hasFocus = true;
    if (IsPointInCloseButton(fx, fy)) { m_state.visible = false; m_state.focused = false; m_state.hasFocus = false; m_host.RequestHersheyCommandRedraw(); return true; }
    if (IsPointInMinimizeButton(fx, fy)) { m_state.minimized = !m_state.minimized; ClampWindow(); m_host.RequestHersheyCommandRedraw(); return true; }
    if (!m_state.minimized && IsPointInScrollbar(fx, fy)) { m_state.draggingScrollbar = true; return true; }
    if (!m_state.minimized && IsPointInTextArea(fx, fy)) { m_state.selectionStart = HitTestTextLine(fx, fy); m_state.selectionEnd = m_state.selectionStart; m_state.selecting = true; m_host.RequestHersheyCommandRedraw(); return true; }
    if (IsPointInTitleBar(fx, fy)) { m_state.dragging = !m_state.dockToBottom; m_state.dragOffset = glm::vec2(fx, fy) - m_state.origin; return true; }
    return true;
}

void HersheyCommandWindow::HandleLeftButtonUp() { m_state.dragging = false; m_state.draggingScrollbar = false; m_state.selecting = false; m_state.resizing = false; }
bool HersheyCommandWindow::HandleMouseWheel(int x, int y, int wheelDelta) { if (!m_state.visible || !IsPointInWindow((float)x, (float)y)) return false; ScrollBy((wheelDelta > 0) ? -3 : 3); return true; }

bool HersheyCommandWindow::HandleKeyDown(unsigned int vk, bool ctrlHeld, bool shiftHeld)
{
    if (!m_state.focused) return false;
    if (ctrlHeld && vk == 'C') { if (shiftHeld) CopyTextToClipboard(BuildAllText()); else CopyTextToClipboard(BuildSelectedText()); return true; }
    if (ctrlHeld && vk == 'A') { m_state.selectionStart = 0; m_state.selectionEnd = std::max(0, TotalCommandLineCount() - 1); m_host.RequestHersheyCommandRedraw(); return true; }
    switch (vk)
    {
    case VK_RETURN: { const std::string submitted = m_state.inputBuffer; m_host.OnHersheyCommandSubmitted(submitted); m_state.inputBuffer.clear(); m_host.RequestHersheyCommandRedraw(); return true; }
    case VK_BACK: if (!m_state.inputBuffer.empty()) { m_state.inputBuffer.pop_back(); m_host.RequestHersheyCommandRedraw(); } return true;
    case VK_UP: ScrollBy(-1); return true;
    case VK_DOWN: ScrollBy(1); return true;
    case VK_PRIOR: ScrollBy(-VisibleCommandLineCapacity()); return true;
    case VK_NEXT: ScrollBy(VisibleCommandLineCapacity()); return true;
    case VK_OEM_3: ToggleVisible(); m_host.RequestHersheyCommandRedraw(); return true;
    default: return false;
    }
}

bool HersheyCommandWindow::HandleChar(unsigned int ch)
{
    if (!m_state.focused) return false;
    if (ch >= 32 && ch <= 126) { m_state.inputBuffer.push_back(static_cast<char>(ch)); m_host.RequestHersheyCommandRedraw(); return true; }
    return false;
}

bool HersheyCommandWindow::WantsTextCursor(int x, int y) const { return m_state.visible && IsPointInWindow((float)x, (float)y); }
bool HersheyCommandWindow::IsVisible() const { return m_state.visible; }
bool HersheyCommandWindow::IsHovered() const { return m_state.hovered; }
bool HersheyCommandWindow::IsFocused() const { return m_state.focused; }
void HersheyCommandWindow::SetVisible(bool visible) { m_state.visible = visible; m_state.focused = visible; m_state.hasFocus = visible; if (visible) ClampWindow(); }
void HersheyCommandWindow::ToggleVisible() { SetVisible(!m_state.visible); if (m_state.visible) m_state.minimized = false; }
void HersheyCommandWindow::SetFocused(bool focused) { m_state.focused = focused; m_state.hasFocus = focused; }

void HersheyCommandWindow::AddHistory(const std::string& line) { m_state.history.push_back(line); while ((int)m_state.history.size() > m_state.backscrollLimit) m_state.history.erase(m_state.history.begin()); const int maxScroll = std::max(0, TotalCommandLineCount() - VisibleCommandLineCapacity()); m_state.scrollOffsetLines = maxScroll; }
void HersheyCommandWindow::ClearHistory() { m_state.history.clear(); m_state.selectionStart = -1; m_state.selectionEnd = -1; m_state.scrollOffsetLines = 0; }
void HersheyCommandWindow::AdvanceBackgroundPalette() { m_state.backgroundIndex = (m_state.backgroundIndex + 1) % (int)m_state.backgroundPalette.size(); }
void HersheyCommandWindow::SetInputBuffer(std::string text) { m_state.inputBuffer = std::move(text); }
const std::string& HersheyCommandWindow::GetInputBuffer() const { return m_state.inputBuffer; }
HersheyCommandWindow::State& HersheyCommandWindow::GetState() { return m_state; }
const HersheyCommandWindow::State& HersheyCommandWindow::GetState() const { return m_state; }

float HersheyCommandWindow::CommandTextLineHeightPx() const
{
    if (!m_font) return 18.0f;
    TextEntity probe; probe.font = m_font; probe.text = "Ag"; probe.scale = m_state.textScale; probe.strokeWidth = m_state.textStroke;
    const HersheyTextBuilder::TextExtents ext = HersheyTextBuilder::MeasureTextUnwrapped(probe);
    return std::max(14.0f, ext.height + 2.0f);
}
float HersheyCommandWindow::CommandInputBandHeightPx() const { return CommandTextLineHeightPx() + 10.0f; }
int HersheyCommandWindow::VisibleCommandLineCapacity() const { if (m_state.minimized) return 0; const float contentHeight = m_state.size.y - m_state.titleBarHeight - (m_state.padding * 2.0f) - CommandInputBandHeightPx(); return std::max(1, (int)std::floor(contentHeight / CommandTextLineHeightPx())); }
int HersheyCommandWindow::TotalCommandLineCount() const
{
    return (int)m_state.history.size();
}
std::string HersheyCommandWindow::GetPromptLine() const { return m_promptLineProvider ? m_promptLineProvider() : std::string("> ") + m_state.inputBuffer; }

std::vector<std::string> HersheyCommandWindow::GetVisibleCommandLines() const
{
    const std::vector<std::string>& all = m_state.history;
    const int visible = VisibleCommandLineCapacity();
    const int total = (int)all.size();
    const int maxStart = std::max(0, total - visible);
    const int start = std::clamp(m_state.scrollOffsetLines, 0, maxStart);
    const int end = std::min(total, start + visible);
    return std::vector<std::string>(all.begin() + start, all.begin() + end);
}

void HersheyCommandWindow::ClampWindow()
{
    const float minExpandedHeight = std::max((float)m_state.minimumHeightPx, m_state.titleBarHeight + CommandInputBandHeightPx() + (m_state.padding * 2.0f) + CommandTextLineHeightPx());
    m_state.expandedHeightPx = std::max(m_state.expandedHeightPx, (int)std::ceil(minExpandedHeight));
    m_state.size.x = (float)std::max(1, m_clientWidth);
    m_state.size.y = m_state.minimized ? (m_state.titleBarHeight + 2.0f) : std::clamp((float)m_state.expandedHeightPx, minExpandedHeight, std::max(minExpandedHeight, (float)m_clientHeight * 0.65f));
    if (m_state.dockToBottom) { m_state.origin.x = 0.0f; m_state.origin.y = std::max(0.0f, (float)m_clientHeight - m_state.size.y); }
    else { m_state.origin.x = std::clamp(m_state.origin.x, 0.0f, std::max(0.0f, (float)m_clientWidth - m_state.size.x)); m_state.origin.y = std::clamp(m_state.origin.y, 0.0f, std::max(0.0f, (float)m_clientHeight - m_state.size.y)); }
}

bool HersheyCommandWindow::IsPointInWindow(float x, float y) const { return m_state.visible && x >= m_state.origin.x && x <= (m_state.origin.x + m_state.size.x) && y >= m_state.origin.y && y <= (m_state.origin.y + m_state.size.y); }
bool HersheyCommandWindow::IsPointInTitleBar(float x, float y) const { if (!IsPointInWindow(x,y) || y > (m_state.origin.y + m_state.titleBarHeight)) return false; return !IsPointInMinimizeButton(x,y) && !IsPointInCloseButton(x,y); }
bool HersheyCommandWindow::IsPointInScrollbar(float x, float y) const { if (!IsPointInWindow(x,y)) return false; const float right = m_state.origin.x + m_state.size.x; return x >= (right - m_state.scrollbarWidth - 2.0f) && y > (m_state.origin.y + m_state.titleBarHeight); }
bool HersheyCommandWindow::IsPointInTextArea(float x, float y) const { if (!IsPointInWindow(x,y)) return false; const float left = m_state.origin.x + m_state.padding; const float top = m_state.origin.y + m_state.titleBarHeight + m_state.padding; const float right = m_state.origin.x + m_state.size.x - m_state.scrollbarWidth - (m_state.padding * 1.5f); const float bottom = m_state.origin.y + m_state.size.y - m_state.padding - CommandInputBandHeightPx(); return x >= left && x <= right && y >= top && y <= bottom; }
bool HersheyCommandWindow::IsPointInMinimizeButton(float x, float y) const { if (!m_state.visible) return false; const float right = m_state.origin.x + m_state.size.x; const float left = right - (m_state.padding + m_state.buttonSize) * 2.0f; const float top = m_state.origin.y + 4.0f; const float bottom = top + m_state.buttonSize; return x >= left && x <= left + m_state.buttonSize && y >= top && y <= bottom; }
bool HersheyCommandWindow::IsPointInCloseButton(float x, float y) const { if (!m_state.visible) return false; const float right = m_state.origin.x + m_state.size.x; const float left = right - m_state.padding - m_state.buttonSize; const float top = m_state.origin.y + 4.0f; const float bottom = top + m_state.buttonSize; return x >= left && x <= left + m_state.buttonSize && y >= top && y <= bottom; }
int HersheyCommandWindow::HitTestTextLine(float, float y) const { const float textTop = m_state.origin.y + m_state.titleBarHeight + m_state.padding; const float lineHeight = CommandTextLineHeightPx(); const int visibleStart = std::clamp(m_state.scrollOffsetLines, 0, std::max(0, TotalCommandLineCount() - VisibleCommandLineCapacity())); const int localIndex = (int)std::floor((y - textTop) / lineHeight); return std::clamp(visibleStart + localIndex, 0, std::max(0, TotalCommandLineCount() - 1)); }

std::string HersheyCommandWindow::BuildSelectedText() const
{
    if (m_state.selectionStart < 0 || m_state.selectionEnd < 0) return {};
    std::vector<std::string> all = m_state.history; all.push_back(GetPromptLine());
    const int a = std::min(m_state.selectionStart, m_state.selectionEnd), b = std::max(m_state.selectionStart, m_state.selectionEnd);
    std::ostringstream oss; for (int i = a; i <= b && i < (int)all.size(); ++i) { if (i > a) oss << "\r\n"; oss << all[i]; } return oss.str();
}

std::string HersheyCommandWindow::BuildAllText() const
{
    std::ostringstream oss; for (std::size_t i = 0; i < m_state.history.size(); ++i) { if (i > 0) oss << "\r\n"; oss << m_state.history[i]; }
    const std::string promptLine = GetPromptLine(); if (!promptLine.empty()) { if (!m_state.history.empty()) oss << "\r\n"; oss << promptLine; } return oss.str();
}

void HersheyCommandWindow::CopyTextToClipboard(const std::string& text) const
{
    if (text.empty()) return; if (!OpenClipboard(nullptr)) return; EmptyClipboard(); HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (mem) { void* dst = GlobalLock(mem); if (dst) { std::memcpy(dst, text.c_str(), text.size() + 1); GlobalUnlock(mem); SetClipboardData(CF_TEXT, mem); mem = nullptr; } }
    if (mem) GlobalFree(mem); CloseClipboard();
}

void HersheyCommandWindow::ScrollBy(int deltaLines) { const int maxScroll = std::max(0, TotalCommandLineCount() - VisibleCommandLineCapacity()); m_state.scrollOffsetLines = std::clamp(m_state.scrollOffsetLines + deltaLines, 0, maxScroll); m_host.RequestHersheyCommandRedraw(); }

void HersheyCommandWindow::AddFilledRect(float x0, float y0, float x1, float y1, const glm::vec4& color, std::vector<UiVertex>& outTriangles) const
{ const glm::vec3 a(x0,y0,0.0f), b(x1,y0,0.0f), c(x1,y1,0.0f), d(x0,y1,0.0f); outTriangles.push_back({a,color}); outTriangles.push_back({b,color}); outTriangles.push_back({c,color}); outTriangles.push_back({a,color}); outTriangles.push_back({c,color}); outTriangles.push_back({d,color}); }
void HersheyCommandWindow::AddLinePx(float x0, float y0, float x1, float y1, const glm::vec4& color, std::vector<UiVertex>& outLines) const { outLines.push_back({glm::vec3(x0,y0,0.0f),color}); outLines.push_back({glm::vec3(x1,y1,0.0f),color}); }
void HersheyCommandWindow::AppendTextBlock(const std::string& text, float x, float y, const glm::vec4& color, float boxWidth, float boxHeight, bool wordWrap, std::vector<UiVertex>& outLines) const
{
    if (!m_font) return; TextEntity te; te.font = m_font; te.text = text; te.position = glm::vec3(x,y,0.0f); te.boxWidth = boxWidth; te.boxHeight = boxHeight; te.wordWrapEnabled = wordWrap; te.hAlign = TextHAlign::Left; te.scale = m_state.textScale; te.strokeWidth = m_state.textStroke; te.color = color; std::vector<LineEntity> lines; HersheyTextBuilder::BuildLines(te, lines); for (const LineEntity& line : lines) { outLines.push_back({line.p0, line.color}); outLines.push_back({line.p1, line.color}); }
}

void HersheyCommandWindow::BuildOverlay(std::vector<UiVertex>& outLines, std::vector<UiVertex>& outTriangles) const
{
    if (!m_state.visible) return; const_cast<HersheyCommandWindow*>(this)->ClampWindow();
    const float fillLeft = m_state.origin.x, fillTop = m_state.origin.y, fillRight = fillLeft + m_state.size.x, fillBottom = fillTop + m_state.size.y;
    const float left = fillLeft + 0.5f, top = fillTop + 0.5f, right = fillRight - 0.5f, bottom = fillBottom - 0.5f;
    const float headerBottom = top + m_state.titleBarHeight, lineHeight = CommandTextLineHeightPx(), inputBandHeight = CommandInputBandHeightPx();
    const bool showContent = !m_state.minimized; const float scrollbarLeft = right - m_state.scrollbarWidth - 2.0f; const float textLeft = left + m_state.padding; const float textTop = headerBottom + m_state.padding; const float textRight = scrollbarLeft - m_state.padding; const float inputTop = bottom - m_state.padding - inputBandHeight; const float inputBaselineY = inputTop + 3.0f;
    const glm::vec4 outline(0.30f, 0.95f, 0.35f, 1.0f), textColor(0.25f, 1.0f, 0.30f, 1.0f), dimText(0.60f, 0.95f, 0.65f, 1.0f); const glm::vec4 bg = m_state.backgroundPalette[m_state.backgroundIndex]; const glm::vec4 headerColor(bg.r + 0.08f, bg.g + 0.08f, bg.b + 0.08f, std::min(0.95f, bg.a + 0.12f)); const glm::vec4 selectionColor(0.25f, 0.55f, 0.90f, 0.25f), scrollbarColor(0.75f, 0.75f, 0.80f, 0.85f);
    AddFilledRect(fillLeft, fillTop, fillRight, fillBottom, bg, outTriangles); AddFilledRect(fillLeft, fillTop, fillRight, headerBottom + 0.5f, headerColor, outTriangles); AddLinePx(left, top, right, top, outline, outLines); AddLinePx(right, top, right, bottom, outline, outLines); AddLinePx(right, bottom, left, bottom, outline, outLines); AddLinePx(left, bottom, left, top, outline, outLines); AddLinePx(left, headerBottom, right, headerBottom, outline, outLines);
    const float minButtonLeft = right - (m_state.padding + m_state.buttonSize) * 2.0f, closeButtonLeft = right - m_state.padding - m_state.buttonSize, buttonTop = top + 4.0f, buttonBottom = buttonTop + m_state.buttonSize;
    AddFilledRect(minButtonLeft, buttonTop, minButtonLeft + m_state.buttonSize, buttonBottom, glm::vec4(0.14f, 0.20f, 0.14f, 0.85f), outTriangles); AddFilledRect(closeButtonLeft, buttonTop, closeButtonLeft + m_state.buttonSize, buttonBottom, glm::vec4(0.22f, 0.10f, 0.10f, 0.85f), outTriangles); AddLinePx(minButtonLeft + 3.0f, buttonBottom - 4.0f, minButtonLeft + m_state.buttonSize - 3.0f, buttonBottom - 4.0f, outline, outLines); AddLinePx(closeButtonLeft + 3.0f, buttonTop + 3.0f, closeButtonLeft + m_state.buttonSize - 3.0f, buttonBottom - 3.0f, outline, outLines); AddLinePx(closeButtonLeft + m_state.buttonSize - 3.0f, buttonTop + 3.0f, closeButtonLeft + 3.0f, buttonBottom - 3.0f, outline, outLines);
    AppendTextBlock(m_title, left + 8.0f, top + 1.0f, textColor, std::max(40.0f, minButtonLeft - left - 12.0f), m_state.titleBarHeight, false, outLines);
    const std::string statusLine = m_statusLineProvider ? m_statusLineProvider() : (std::string(m_state.hasFocus ? "FOCUS " : "") + (m_state.hasMouseFocus ? "MOUSE " : "") + (m_state.minimized ? "MIN" : "READY"));
    AppendTextBlock(statusLine, left + 240.0f, top + 1.0f, dimText, std::max(40.0f, minButtonLeft - left - 244.0f), m_state.titleBarHeight, false, outLines);
    if (!showContent) return;
    const std::vector<std::string> visibleLines = GetVisibleCommandLines(); const int visibleStart = std::clamp(m_state.scrollOffsetLines, 0, std::max(0, TotalCommandLineCount() - VisibleCommandLineCapacity()));
    if (m_state.selectionStart >= 0 && m_state.selectionEnd >= 0) { const int selA = std::min(m_state.selectionStart, m_state.selectionEnd), selB = std::max(m_state.selectionStart, m_state.selectionEnd); for (std::size_t i = 0; i < visibleLines.size(); ++i) { const int globalIndex = visibleStart + (int)i; if (globalIndex < selA || globalIndex > selB) continue; const float y0 = textTop + ((float)i * lineHeight); AddFilledRect(textLeft - 2.0f, y0, textRight, y0 + lineHeight, selectionColor, outTriangles); } }
    for (std::size_t i = 0; i < visibleLines.size(); ++i) { const float y = textTop + ((float)i * lineHeight); AppendTextBlock(visibleLines[i], textLeft, y, textColor, textRight - textLeft, lineHeight, false, outLines); }
    AddLinePx(textLeft, inputTop, textRight, inputTop, outline, outLines); const std::string promptLine = GetPromptLine(); AppendTextBlock(promptLine, textLeft, inputBaselineY, textColor, textRight - textLeft, inputBandHeight, false, outLines);
    const bool blinkOn = std::fmod(m_elapsedSeconds, 1.0f) < 0.5f; if (blinkOn && m_state.focused && m_font) { const float cursorX = textLeft + HersheyTextBuilder::MeasureTextWidth(promptLine, m_font, m_state.textScale) + 2.0f; const float cursorTop = inputTop + 4.0f, cursorBottom = inputTop + inputBandHeight - 4.0f; AddLinePx(cursorX, cursorTop, cursorX, cursorBottom, textColor, outLines); AddLinePx(cursorX - 2.0f, cursorTop, cursorX + 2.0f, cursorTop, textColor, outLines); AddLinePx(cursorX - 2.0f, cursorBottom, cursorX + 2.0f, cursorBottom, textColor, outLines); }
    const float trackTop = textTop, trackBottom = inputTop - m_state.padding; AddLinePx(scrollbarLeft, trackTop, scrollbarLeft, trackBottom, outline, outLines); AddLinePx(right - 2.0f, trackTop, right - 2.0f, trackBottom, outline, outLines);
    const int visibleCount = std::max(1, VisibleCommandLineCapacity()), totalCount = std::max(visibleCount, TotalCommandLineCount()); const float trackHeight = std::max(1.0f, trackBottom - trackTop); const float thumbHeight = std::max(20.0f, trackHeight * ((float)visibleCount / (float)totalCount)); const int maxScroll = std::max(0, totalCount - visibleCount); const float travel = std::max(1.0f, trackHeight - thumbHeight); const float thumbTop = trackTop + ((maxScroll > 0) ? (travel * ((float)m_state.scrollOffsetLines / (float)maxScroll)) : 0.0f); AddFilledRect(scrollbarLeft + 2.0f, thumbTop, right - 4.0f, thumbTop + thumbHeight, scrollbarColor, outTriangles);
}
}
