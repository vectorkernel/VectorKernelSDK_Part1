#pragma once
#include <string>
#include <vector>
#include "TextEntity.h"
#include "LineEntity.h"

// Converts a TextEntity into a list of LineEntity segments (GL_LINES).
// Output is appended to outLines.
namespace HersheyTextBuilder
{
    struct TextExtents
    {
        float width = 0.0f;
        float height = 0.0f;
        std::size_t lineCount = 0;
    };

    // Measure a single-line string width in text units (no wrapping).
    float MeasureTextWidth(const std::string& s, hershey_font* font, float scale);

    // Measure using explicit lines only (respects '\n', ignores word wrap entirely).
    TextExtents MeasureTextUnwrapped(const TextEntity& text);

    // Measure using the current layout rules (respects wrapping and box width).
    TextExtents MeasureTextLaidOut(const TextEntity& text);

    // Split + wrap into lines (respects '\n' and TextEntity.wordWrapEnabled/boxWidth).
    std::vector<std::string> BuildTextLines(const TextEntity& text);

    // Expand the text into line segments.
    // The text box origin is interpreted in overlay/UI coordinates with origin at the
    // upper-left and Y increasing downward.
    void BuildLines(const TextEntity& text, std::vector<LineEntity>& outLines);
}
