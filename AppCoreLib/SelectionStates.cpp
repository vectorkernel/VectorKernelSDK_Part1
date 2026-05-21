#include "pch.h"

#include "SelectionStates.h"
#include "SelectionController.h"

namespace AppCore
{
    SelectionStateKind ClassifySelectionState(const SelectionController& selection, std::size_t entityId)
    {
        if (entityId == 0)
            return SelectionStateKind::None;

        if (selection.IsGripsSelectionMode() && selection.Contains(entityId))
            return SelectionStateKind::GripSelected;

        if (selection.Contains(entityId))
            return SelectionStateKind::Selected;

        if (selection.HasHoverHit() && selection.GetHoverEntityId() == entityId)
            return SelectionStateKind::Hovered;

        return SelectionStateKind::None;
    }
}
