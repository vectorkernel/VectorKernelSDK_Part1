#include "pch.h"

#include "GripsOps.h"
#include "InteractionState.h"
#include "EntityBook.h"
#include "Entity.h"
#include "LineEntity.h"
#include "HoverOps.h" // SegmentIntersectsAabb2D

#include <unordered_set>

namespace AppCore
{
    void ClearGrips(InteractionState& s)
    {
        if (s.gripsIds.empty())
            return;

        s.gripsIds.clear();
        s.requestRedraw = true;
    }

    bool ApplyGripsFromAabb(EntityBook& book, InteractionState& s,
                            const glm::vec2& worldMin, const glm::vec2& worldMax,
                            GripsApplyMode mode,
                            GripsHitPolicy hitPolicy)
    {
        std::unordered_set<std::size_t> hits;
        hits.reserve(64);

        for (const auto& ep : book.entities)
        {
            if (!ep) continue;
            const Entity& e = *ep;
            if (!e.visible) continue;
            if (!e.pickable) continue;
            if (e.type != EntityType::Line) continue;
            if (e.screenSpace) continue;
            if (e.tag != EntityTag::User) continue;

            const auto& line = static_cast<const LineEntity&>(e);
            glm::vec2 a(line.p0.x, line.p0.y);
            glm::vec2 b(line.p1.x, line.p1.y);

            bool include = false;
            if (hitPolicy == GripsHitPolicy::Window)
            {
                const bool aInside = (a.x >= worldMin.x && a.x <= worldMax.x && a.y >= worldMin.y && a.y <= worldMax.y);
                const bool bInside = (b.x >= worldMin.x && b.x <= worldMax.x && b.y >= worldMin.y && b.y <= worldMax.y);
                include = aInside && bInside;
            }
            else
            {
                include = AppCore::SegmentIntersectsAabb2D(a, b, worldMin, worldMax);
            }

            if (include)
                hits.insert(e.ID);
        }

        if (hits.empty() && mode != GripsApplyMode::Replace)
            return false;

        auto before = s.gripsIds;

        switch (mode)
        {
        case GripsApplyMode::Replace:
            s.gripsIds = std::move(hits);
            break;
        case GripsApplyMode::Add:
            for (auto id : hits) s.gripsIds.insert(id);
            break;
        case GripsApplyMode::Toggle:
            for (auto id : hits)
            {
                if (s.gripsIds.find(id) != s.gripsIds.end()) s.gripsIds.erase(id);
                else s.gripsIds.insert(id);
            }
            break;
        }

        const bool changed = (before != s.gripsIds);
        if (changed)
            s.requestRedraw = true;
        return changed;
    }
}
