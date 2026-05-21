#pragma once

#include <cstddef>
#include <unordered_set>

class EntityBook;
struct InteractionState;

namespace AppCore
{
    enum class VisualSelectionState
    {
        Unselected,
        Selected,
        GripSelected
    };

    inline bool ContainsId(const std::unordered_set<std::size_t>& ids, std::size_t id)
    {
        return ids.find(id) != ids.end();
    }

    inline bool IsGripSelected(const InteractionState& s, std::size_t id)
    {
        return ContainsId(s.gripsIds, id);
    }

    inline bool IsSelected(const EntityBook& book, std::size_t id);
    inline void SyncGripsToSelection(const EntityBook& book, InteractionState& s);
    inline VisualSelectionState GetVisualSelectionState(const EntityBook& book,
                                                        const InteractionState& s,
                                                        std::size_t id);
}

#include "..\EntityCoreLib\EntityBook.h"
#include "InteractionState.h"

namespace AppCore
{
    inline bool IsSelected(const EntityBook& book, std::size_t id)
    {
        const auto& selectedIds = book.SelectedIds();
        for (std::size_t selectedId : selectedIds)
            if (selectedId == id)
                return true;
        return false;
    }

    inline void SyncGripsToSelection(const EntityBook& book, InteractionState& s)
    {
        s.gripsIds.clear();
        for (std::size_t id : book.SelectedIds())
            s.gripsIds.insert(id);
        s.requestRedraw = true;
    }

    inline VisualSelectionState GetVisualSelectionState(const EntityBook& book,
                                                        const InteractionState& s,
                                                        std::size_t id)
    {
        if (IsGripSelected(s, id))
            return VisualSelectionState::GripSelected;
        if (IsSelected(book, id))
            return VisualSelectionState::Selected;
        return VisualSelectionState::Unselected;
    }
}
