#pragma once

#include <unordered_set>

#include <glm/glm.hpp>

#include "Entity.h"
#include "LineEntity.h"
#include "..\AppCoreLib\SelectionVisualState.h"
#include "InteractionState.h"
#include "EntityBook.h"

namespace RenderCore
{
    inline glm::vec4 EffectiveLineColor(const Entity& e,
                                        const EntityBook* book,
                                        const InteractionState* interaction,
                                        const glm::vec4& hoverColor,
                                        const glm::vec4& selectedColor,
                                        const glm::vec4& gripSelectedColor)
    {
        const auto& line = static_cast<const LineEntity&>(e);
        glm::vec4 c = line.color;

        if (!e.visible)
            return c;

        if (!e.screenSpace && e.tag == EntityTag::User)
        {
            const bool hovered = interaction && (interaction->hoveredIds.find(e.ID) != interaction->hoveredIds.end());
            if (hovered)
                return hoverColor;

            if (book && interaction)
            {
                switch (AppCore::GetVisualSelectionState(*book, *interaction, e.ID))
                {
                case AppCore::VisualSelectionState::GripSelected: return gripSelectedColor;
                case AppCore::VisualSelectionState::Selected:     return selectedColor;
                default:                                          break;
                }
            }
            else if (e.selected)
            {
                return selectedColor;
            }
        }

        return c;
    }
}
