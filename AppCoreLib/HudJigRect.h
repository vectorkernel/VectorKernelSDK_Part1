#pragma once

#include <glm/glm.hpp>

namespace AppCore
{
    enum class HudJigKind
    {
        None = 0,
        ZoomWindow
    };

    struct HudJigRect
    {
        HudJigKind kind = HudJigKind::None;
        bool armed = false;
        bool dragging = false;
        glm::vec2 startWorld{ 0.0f, 0.0f };
        glm::vec2 currentWorld{ 0.0f, 0.0f };

        void Arm(HudJigKind inKind)
        {
            kind = inKind;
            armed = true;
            dragging = false;
            startWorld = glm::vec2(0.0f, 0.0f);
            currentWorld = glm::vec2(0.0f, 0.0f);
        }

        void Begin(const glm::vec2& world)
        {
            if (kind == HudJigKind::None)
                return;

            armed = true;
            dragging = true;
            startWorld = world;
            currentWorld = world;
        }

        void Update(const glm::vec2& world)
        {
            if (!armed)
                return;
            currentWorld = world;
        }

        void Cancel()
        {
            kind = HudJigKind::None;
            armed = false;
            dragging = false;
            startWorld = glm::vec2(0.0f, 0.0f);
            currentWorld = glm::vec2(0.0f, 0.0f);
        }

        [[nodiscard]] bool IsActive() const
        {
            return kind != HudJigKind::None && armed;
        }

        [[nodiscard]] bool HasDragRect(float epsilon = 1.0e-5f) const
        {
            const glm::vec2 delta = currentWorld - startWorld;
            return IsActive() && ((glm::abs(delta.x) > epsilon) || (glm::abs(delta.y) > epsilon));
        }
    };
}
