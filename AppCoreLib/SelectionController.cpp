#include "pch.h"

#include "SelectionController.h"

void SelectionController::SetMode(SelectionMode mode) noexcept
{
    m_mode = mode;
}

SelectionMode SelectionController::GetMode() const noexcept
{
    return m_mode;
}

bool SelectionController::IsPlainSelectionMode() const noexcept
{
    return m_mode == SelectionMode::Plain;
}

bool SelectionController::IsGripsSelectionMode() const noexcept
{
    return m_mode == SelectionMode::Grips;
}

void SelectionController::ClearSelection() noexcept
{
    m_selected.clear();
}

bool SelectionController::HasSelection() const noexcept
{
    return !m_selected.empty();
}

std::size_t SelectionController::GetSelectionCount() const noexcept
{
    return m_selected.size();
}

void SelectionController::Add(std::size_t id)
{
    if (id != 0)
        m_selected.insert(id);
}

void SelectionController::Remove(std::size_t id)
{
    m_selected.erase(id);
}

void SelectionController::Toggle(std::size_t id)
{
    if (id == 0)
        return;

    auto it = m_selected.find(id);
    if (it != m_selected.end())
        m_selected.erase(it);
    else
        m_selected.insert(id);
}

bool SelectionController::Contains(std::size_t id) const
{
    return m_selected.find(id) != m_selected.end();
}

void SelectionController::AddRange(const std::vector<std::size_t>& ids)
{
    for (std::size_t id : ids)
        Add(id);
}

const std::unordered_set<std::size_t>& SelectionController::GetSelectedEntities() const noexcept
{
    return m_selected;
}

void SelectionController::SetHoverHit(const SelectionHit& hit) noexcept
{
    m_hoverHit = hit;
}

void SelectionController::ClearHover() noexcept
{
    m_hoverHit.Reset();
}

const SelectionHit& SelectionController::GetHoverHit() const noexcept
{
    return m_hoverHit;
}

std::size_t SelectionController::GetHoverEntityId() const noexcept
{
    return m_hoverHit.id;
}

bool SelectionController::HasHoverHit() const noexcept
{
    return m_hoverHit.IsValid();
}

void SelectionController::BeginCrossingBox(const glm::vec2& startWorld) noexcept
{
    m_boxSelection.active = true;
    m_boxSelection.crossing = true;
    m_boxSelection.startWorld = startWorld;
    m_boxSelection.currentWorld = startWorld;
}

void SelectionController::BeginWindowBox(const glm::vec2& startWorld) noexcept
{
    m_boxSelection.active = true;
    m_boxSelection.crossing = false;
    m_boxSelection.startWorld = startWorld;
    m_boxSelection.currentWorld = startWorld;
}

void SelectionController::UpdateBox(const glm::vec2& currentWorld) noexcept
{
    if (!m_boxSelection.active)
        return;

    m_boxSelection.currentWorld = currentWorld;
}

void SelectionController::EndBox() noexcept
{
    m_boxSelection.active = false;
}

void SelectionController::CancelBox() noexcept
{
    m_boxSelection.Reset();
}

bool SelectionController::IsBoxSelectionActive() const noexcept
{
    return m_boxSelection.active;
}

bool SelectionController::IsCrossingBoxSelection() const noexcept
{
    return m_boxSelection.active && m_boxSelection.crossing;
}

bool SelectionController::IsWindowBoxSelection() const noexcept
{
    return m_boxSelection.active && !m_boxSelection.crossing;
}

const BoxSelectionState& SelectionController::GetBoxSelectionState() const noexcept
{
    return m_boxSelection;
}
