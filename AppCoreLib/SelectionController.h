#pragma once

#include <cstddef>
#include <unordered_set>
#include <vector>

#include "SelectionHit.h"
#include "BoxSelectionState.h"
#include "SelectionMode.h"

class SelectionController
{
public:
    void SetMode(SelectionMode mode) noexcept;
    [[nodiscard]] SelectionMode GetMode() const noexcept;
    [[nodiscard]] bool IsPlainSelectionMode() const noexcept;
    [[nodiscard]] bool IsGripsSelectionMode() const noexcept;

    void ClearSelection() noexcept;
    [[nodiscard]] bool HasSelection() const noexcept;
    [[nodiscard]] std::size_t GetSelectionCount() const noexcept;

    void Add(std::size_t id);
    void Remove(std::size_t id);
    void Toggle(std::size_t id);
    [[nodiscard]] bool Contains(std::size_t id) const;

    void AddRange(const std::vector<std::size_t>& ids);
    [[nodiscard]] const std::unordered_set<std::size_t>& GetSelectedEntities() const noexcept;

    void SetHoverHit(const SelectionHit& hit) noexcept;
    void ClearHover() noexcept;
    [[nodiscard]] const SelectionHit& GetHoverHit() const noexcept;
    [[nodiscard]] std::size_t GetHoverEntityId() const noexcept;
    [[nodiscard]] bool HasHoverHit() const noexcept;

    void BeginCrossingBox(const glm::vec2& startWorld) noexcept;
    void BeginWindowBox(const glm::vec2& startWorld) noexcept;
    void UpdateBox(const glm::vec2& currentWorld) noexcept;
    void EndBox() noexcept;
    void CancelBox() noexcept;

    [[nodiscard]] bool IsBoxSelectionActive() const noexcept;
    [[nodiscard]] bool IsCrossingBoxSelection() const noexcept;
    [[nodiscard]] bool IsWindowBoxSelection() const noexcept;
    [[nodiscard]] const BoxSelectionState& GetBoxSelectionState() const noexcept;

private:
    SelectionMode m_mode = SelectionMode::Plain;
    std::unordered_set<std::size_t> m_selected;
    SelectionHit m_hoverHit;
    BoxSelectionState m_boxSelection;
};
