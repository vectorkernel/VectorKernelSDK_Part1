#pragma once

#include <glm/vec2.hpp>

namespace AppCoreLib
{
    struct ViewportTransform
    {
        float screenWidth = 1.0f;
        float screenHeight = 1.0f;
        float modelHalfWidth = 1.0f;
        float modelHalfHeight = 1.0f;

        glm::vec2 ScreenToModel(glm::vec2 screenPx) const
        {
            const float nx = (screenPx.x / screenWidth) * 2.0f - 1.0f;
            const float ny = 1.0f - (screenPx.y / screenHeight) * 2.0f;
            return { nx * modelHalfWidth, ny * modelHalfHeight };
        }

        glm::vec2 ModelToScreen(glm::vec2 model) const
        {
            const float nx = model.x / modelHalfWidth;
            const float ny = model.y / modelHalfHeight;
            return {
                (nx * 0.5f + 0.5f) * screenWidth,
                (0.5f - ny * 0.5f) * screenHeight
            };
        }

        glm::vec2 PixelsToModelDelta(glm::vec2 pixelDelta) const
        {
            return {
                pixelDelta.x * ((2.0f * modelHalfWidth) / screenWidth),
                -pixelDelta.y * ((2.0f * modelHalfHeight) / screenHeight)
            };
        }
    };
}
