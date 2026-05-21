#pragma once

#include <glm/glm.hpp>

struct BoxSelectionState
{
    bool active = false;
    bool crossing = false;
    glm::vec2 startWorld = {};
    glm::vec2 currentWorld = {};

    void Reset() noexcept
    {
        active = false;
        crossing = false;
        startWorld = {};
        currentWorld = {};
    }
};
