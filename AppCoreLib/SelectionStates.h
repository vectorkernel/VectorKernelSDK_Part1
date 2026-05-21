#pragma once

#include <cstddef>

class EntityBook;
class SelectionController;

namespace AppCore
{
    enum class SelectionStateKind
    {
        None = 0,
        Hovered,
        Selected,
        GripSelected
    };

    SelectionStateKind ClassifySelectionState(const SelectionController& selection, std::size_t entityId);
}
