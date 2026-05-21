#pragma once

struct ClientSize
{
    int width = 0;
    int height = 0;

    [[nodiscard]] bool IsValid() const
    {
        return width > 0 && height > 0;
    }

    [[nodiscard]] float AspectRatioOr(float fallback = 1.0f) const
    {
        return (height != 0) ? static_cast<float>(width) / static_cast<float>(height) : fallback;
    }

    [[nodiscard]] bool operator==(const ClientSize& other) const
    {
        return width == other.width && height == other.height;
    }

    [[nodiscard]] bool operator!=(const ClientSize& other) const
    {
        return !(*this == other);
    }
};
