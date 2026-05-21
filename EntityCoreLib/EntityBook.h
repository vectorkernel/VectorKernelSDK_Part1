#pragma once
#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "VklExpected.h"
#include "Entity.h"
#include "EntityCoreLog.h"
#include "EntityBookError.h"

class EntityBook;

vkl::expected<std::unique_ptr<EntityBook>, EntityBookError>
CreateEntityBook(std::size_t capacity);

class EntityBook
{
public:
    using EntityPtr = std::unique_ptr<Entity>;
    using EntityList = std::vector<EntityPtr>;

    friend vkl::expected<std::unique_ptr<EntityBook>, EntityBookError>
    CreateEntityBook(std::size_t capacity);

    EntityBook() = default;
    EntityList entities;

    const std::vector<std::size_t>& SelectedIds() const noexcept { return m_selectedIds; }
    void ClearSelection();
    void SetSelected(Entity& e, bool on);
    void Select(Entity& e) { SetSelected(e, true); }
    void Deselect(Entity& e) { SetSelected(e, false); }

    vkl::expected<Entity*, EntityBookError> TryAddEntity(const Entity& e);
    vkl::expected<Entity*, EntityBookError> TryAddEntity(std::unique_ptr<Entity> e);

    Entity& AddEntity(const Entity& e);
    Entity& AddEntity(std::unique_ptr<Entity> e);

    bool ContainsId(std::size_t id) const noexcept;
    Entity* FindById(std::size_t id) noexcept;
    const Entity* FindById(std::size_t id) const noexcept;

    bool CanGrow() const noexcept;
    vkl::expected<bool, EntityBookError> TryResize(std::size_t newCapacity);
    vkl::expected<bool, EntityBookError> TryGrow();

    template <typename Pred>
    void RemoveIf(Pred&& pred)
    {
        const auto before = entities.size();
        auto it = std::remove_if(entities.begin(), entities.end(),
            [&](const EntityPtr& e)
            {
                const bool remove = e && pred(*e);
                if (remove)
                {
                    EntityCore::EmitLog(EntityCore::LogLevel::Trace, "EntityBook", "RemoveEntity", EntityCore::DescribeEntity(*e));
                }
                return remove;
            });

        entities.erase(it, entities.end());

        if (entities.size() != before)
        {
            ++revision;
            RebuildSelectionCache();
        }
    }

    void Clear();

    const EntityList& GetEntities() const;
    EntityList& GetEntitiesMutable();

    void SortByDrawOrder();
    void Touch();
    void Touch(const char* reason);

    const char* GetLastTouchReason() const { return lastTouchReason.c_str(); }
    uint64_t GetRevision() const { return revision; }
    std::size_t Capacity() const noexcept { return m_capacity; }
    void ClearEntityBook();

private:
    explicit EntityBook(std::size_t capacity) : m_capacity(capacity) {}

    void RebuildSelectionCache();
    void ClearSelectionCache();
    bool Initialize();

    uint64_t revision = 1;
    std::string lastTouchReason;
    std::vector<std::size_t> m_selectedIds;
    std::unordered_map<std::size_t, std::size_t> m_selectedIndex;
    std::size_t m_capacity{};
};
