#include "pch.h"

#include "RGeometryTree.h"

void RGeometryTree::Clear()
{
    m_items.clear();
}

void RGeometryTree::Build(const std::vector<std::pair<BoundingBox, std::size_t>>& items)
{
    m_items = items;
}

std::optional<std::size_t> RGeometryTree::QueryFirstIntersect(const BoundingBox& box) const
{
    for (const auto& item : m_items)
    {
        if (Intersects(item.first, box))
            return item.second;
    }

    return std::nullopt;
}
