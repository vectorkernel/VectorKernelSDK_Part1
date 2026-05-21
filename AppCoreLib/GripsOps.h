#pragma once

#include <cstddef>
#include <glm/glm.hpp>

class EntityBook;
struct InteractionState;

namespace AppCore
{
    // How to apply a hit-test result to the grips set.
    enum class GripsApplyMode
    {
        Replace, // clear then add hits
        Add,     // add hits
        Toggle   // toggle each hit
    };

    // Which rectangle-selection rule to use.
    enum class GripsHitPolicy
    {
        Crossing, // any entity intersecting the rectangle
        Window    // entity must lie fully inside the rectangle
    };

    // Clears only the grips selection set (does not touch EntityBook selection).
    void ClearGrips(InteractionState& s);

    // Apply a world-space AABB hit test against pickable entities and update the grips set.
    // Returns true if gripsIds changed.
    bool ApplyGripsFromAabb(EntityBook& book, InteractionState& s,
                            const glm::vec2& worldMin, const glm::vec2& worldMax,
                            GripsApplyMode mode,
                            GripsHitPolicy hitPolicy = GripsHitPolicy::Crossing);
}
