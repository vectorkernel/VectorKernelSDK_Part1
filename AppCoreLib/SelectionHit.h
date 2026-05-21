#pragma once

#include <cstddef>
#include <cfloat>

struct SelectionHit
{
    std::size_t id = 0;
    float score = FLT_MAX;

    [[nodiscard]] bool IsValid() const noexcept
    {
        return id != 0;
    }

    void Reset() noexcept
    {
        id = 0;
        score = FLT_MAX;
    }
};
