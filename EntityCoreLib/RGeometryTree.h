#pragma once
#include <vector>
#include <cstddef>
#include <optional>
#include <utility>

#include "BoundingBox.h"

class RGeometryTree
{
public:
    void Clear();
    void Build(const std::vector<std::pair<BoundingBox, std::size_t>>& items);

    // Query with an AABB (picker square in world space). Returns the first hit (best-effort).
    std::optional<std::size_t> QueryFirstIntersect(const BoundingBox& box) const;

private:
    using Value = std::pair<BoundingBox, std::size_t>;
    std::vector<Value> m_items;
};
