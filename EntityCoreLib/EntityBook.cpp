#include "pch.h"
#include "EntityBook.h"

#include <algorithm>
#include <cstdio>
#include <stdexcept>

#include "EntityCoreLog.h"

bool EntityBook::ContainsId(std::size_t id) const noexcept
{
    return FindById(id) != nullptr;
}

Entity* EntityBook::FindById(std::size_t id) noexcept
{
    for (auto& entity : entities)
    {
        if (entity && entity->ID == id)
        {
            return entity.get();
        }
    }
    return nullptr;
}

const Entity* EntityBook::FindById(std::size_t id) const noexcept
{
    for (const auto& entity : entities)
    {
        if (entity && entity->ID == id)
        {
            return entity.get();
        }
    }
    return nullptr;
}

vkl::expected<Entity*, EntityBookError> EntityBook::TryAddEntity(const Entity& e)
{
    return TryAddEntity(e.Clone());
}

vkl::expected<Entity*, EntityBookError> EntityBook::TryAddEntity(std::unique_ptr<Entity> e)
{
    if (!e)
    {
        EntityBookError err{ EntityBookErrc::InitFailed, "Null entity pointer." };
        EntityCore::EmitLog(EntityCore::LogLevel::Error, "EntityBook", "AddEntity", err.message.c_str());
        return vkl::unexpected(err);
    }

    if (entities.size() >= m_capacity)
    {
        EntityBookError err{ EntityBookErrc::CapacityExceeded, "EntityBook capacity exceeded." };
        EntityCore::EmitLog(EntityCore::LogLevel::Error, "EntityBook", "AddEntity", err.message.c_str());
        return vkl::unexpected(err);
    }

    if (ContainsId(e->ID))
    {
        EntityBookError err{ EntityBookErrc::DuplicateId, "Duplicate entity ID." };
        EntityCore::EmitLog(EntityCore::LogLevel::Error, "EntityBook", "AddEntity", err.message.c_str());
        return vkl::unexpected(err);
    }

    const bool isSelected = e->selected;
    const std::size_t id = e->ID;
    entities.push_back(std::move(e));
    if (isSelected)
    {
        m_selectedIndex.emplace(id, m_selectedIds.size());
        m_selectedIds.push_back(id);
    }
    ++revision;

    EntityCore::EmitLog(EntityCore::LogLevel::Trace, "EntityBook", "AddEntity",
        EntityCore::DescribeEntity(*entities.back()));
    return entities.back().get();
}

bool EntityBook::CanGrow() const noexcept
{
    return m_capacity < (std::numeric_limits<std::size_t>::max() / 2);
}

vkl::expected<bool, EntityBookError> EntityBook::TryResize(std::size_t newCapacity)
{
    if (newCapacity <= m_capacity)
    {
        EntityBookError err{ EntityBookErrc::InvalidCapacity,
            "New EntityBook capacity must be greater than current capacity." };
        EntityCore::EmitLog(EntityCore::LogLevel::Error, "EntityBook", "TryResize", err.message.c_str());
        return vkl::unexpected(err);
    }

    try
    {
        entities.reserve(newCapacity);
    }
    catch (const std::bad_alloc&)
    {
        EntityBookError err{ EntityBookErrc::OutOfMemory,
            "Unable to grow EntityBook: out of memory." };
        EntityCore::EmitLog(EntityCore::LogLevel::Error, "EntityBook", "TryResize", err.message.c_str());
        return vkl::unexpected(err);
    }
    catch (...)
    {
        EntityBookError err{ EntityBookErrc::AllocationFailed,
            "Unable to grow EntityBook: unknown exception." };
        EntityCore::EmitLog(EntityCore::LogLevel::Error, "EntityBook", "TryResize", err.message.c_str());
        return vkl::unexpected(err);
    }

    if (entities.capacity() < newCapacity)
    {
        EntityBookError err{ EntityBookErrc::AllocationFailed,
            "Reserve did not achieve the requested EntityBook capacity." };
        EntityCore::EmitLog(EntityCore::LogLevel::Error, "EntityBook", "TryResize", err.message.c_str());
        return vkl::unexpected(err);
    }

    m_capacity = newCapacity;

    char buf[128];
    std::snprintf(buf, sizeof(buf), "EntityBook capacity increased to %zu", m_capacity);
    EntityCore::EmitLog(EntityCore::LogLevel::Info, "EntityBook", "TryResize", buf);
    return true;
}

vkl::expected<bool, EntityBookError> EntityBook::TryGrow()
{
    if (m_capacity == 0)
    {
        EntityBookError err{ EntityBookErrc::InvalidCapacity,
            "EntityBook cannot grow from zero capacity." };
        EntityCore::EmitLog(EntityCore::LogLevel::Error, "EntityBook", "TryGrow", err.message.c_str());
        return vkl::unexpected(err);
    }

    std::size_t newCapacity = 0;
    if (m_capacity < 1024)
    {
        if (m_capacity > (std::numeric_limits<std::size_t>::max() / 2))
        {
            EntityBookError err{ EntityBookErrc::CapacityExceeded,
                "EntityBook cannot grow any further." };
            EntityCore::EmitLog(EntityCore::LogLevel::Error, "EntityBook", "TryGrow", err.message.c_str());
            return vkl::unexpected(err);
        }
        newCapacity = m_capacity * 2;
    }
    else
    {
        constexpr std::size_t kGrowthStep = 10000;
        if (m_capacity > (std::numeric_limits<std::size_t>::max() - kGrowthStep))
        {
            EntityBookError err{ EntityBookErrc::CapacityExceeded,
                "EntityBook cannot grow any further." };
            EntityCore::EmitLog(EntityCore::LogLevel::Error, "EntityBook", "TryGrow", err.message.c_str());
            return vkl::unexpected(err);
        }
        newCapacity = m_capacity + kGrowthStep;
    }

    return TryResize(newCapacity);
}

Entity& EntityBook::AddEntity(const Entity& e)
{
    if (entities.size() >= m_capacity)
    {
        auto growResult = TryGrow();
        if (!growResult.has_value())
        {
            throw std::runtime_error(growResult.error().message);
        }
    }

    auto result = TryAddEntity(e);
    if (!result.has_value())
    {
        throw std::runtime_error(result.error().message);
    }
    return *result.value();
}

Entity& EntityBook::AddEntity(std::unique_ptr<Entity> e)
{
    if (!e)
    {
        throw std::runtime_error("Null entity pointer.");
    }

    if (entities.size() >= m_capacity)
    {
        auto growResult = TryGrow();
        if (!growResult.has_value())
        {
            throw std::runtime_error(growResult.error().message);
        }
    }

    auto result = TryAddEntity(std::move(e));
    if (!result.has_value())
    {
        throw std::runtime_error(result.error().message);
    }
    return *result.value();
}

void EntityBook::Clear()
{
    entities.clear();
    ClearSelectionCache();
    ++revision;

    EntityCore::EmitLog(EntityCore::LogLevel::Info, "EntityBook", "ClearEntityBook", "");
}

void EntityBook::ClearSelectionCache()
{
    m_selectedIds.clear();
    m_selectedIndex.clear();
}

void EntityBook::RebuildSelectionCache()
{
    ClearSelectionCache();
    m_selectedIds.reserve(64);
    for (const auto& e : entities)
    {
        if (!e || !e->selected) continue;
        if (m_selectedIndex.find(e->ID) != m_selectedIndex.end()) continue;
        m_selectedIndex.emplace(e->ID, m_selectedIds.size());
        m_selectedIds.push_back(e->ID);
    }
}

void EntityBook::SetSelected(Entity& e, bool on)
{
    if (on)
    {
        if (e.selected) return;
        e.selected = true;
        if (m_selectedIndex.find(e.ID) == m_selectedIndex.end())
        {
            m_selectedIndex.emplace(e.ID, m_selectedIds.size());
            m_selectedIds.push_back(e.ID);
        }
        Touch("Selection");
        return;
    }

    if (!e.selected) return;
    e.selected = false;

    auto it = m_selectedIndex.find(e.ID);
    if (it != m_selectedIndex.end())
    {
        const std::size_t idx = it->second;
        const std::size_t lastIdx = (m_selectedIds.empty() ? 0 : (m_selectedIds.size() - 1));
        if (!m_selectedIds.empty() && idx <= lastIdx)
        {
            const std::size_t lastId = m_selectedIds[lastIdx];
            m_selectedIds[idx] = lastId;
            m_selectedIds.pop_back();
            m_selectedIndex.erase(it);
            if (idx != lastIdx)
                m_selectedIndex[lastId] = idx;
        }
    }

    Touch("Selection");
}

void EntityBook::ClearSelection()
{
    if (m_selectedIds.empty())
        return;

    for (auto& e : entities)
        if (e) e->selected = false;

    ClearSelectionCache();
    Touch("ClearSelection");
}

const EntityBook::EntityList& EntityBook::GetEntities() const
{
    return entities;
}

EntityBook::EntityList& EntityBook::GetEntitiesMutable()
{
    return entities;
}

void EntityBook::SortByDrawOrder()
{
    std::stable_sort(entities.begin(), entities.end(),
        [](const EntityPtr& a, const EntityPtr& b)
        {
            if (!a || !b) return static_cast<bool>(a) < static_cast<bool>(b);
            if (a->drawOrder != b->drawOrder) return a->drawOrder < b->drawOrder;

            auto layer = [](EntityTag t)
            {
                switch (t)
                {
                case EntityTag::Grid:   return 0;
                case EntityTag::Scene:  return 1;
                case EntityTag::User:   return 1;
                case EntityTag::Cursor: return 2;
                case EntityTag::Hud:    return 3;
                default:                return 1;
                }
            };

            int la = layer(a->tag);
            int lb = layer(b->tag);
            if (la != lb) return la < lb;

            return a->ID < b->ID;
        });

    ++revision;

    char buf[64];
    std::snprintf(buf, sizeof(buf), "count=%zu", entities.size());
    EntityCore::EmitLog(EntityCore::LogLevel::Trace, "EntityBook", "SortByDrawOrder", buf);
}

void EntityBook::Touch()
{
    Touch("Touch()");
}

void EntityBook::Touch(const char* reason)
{
    lastTouchReason = (reason && *reason) ? reason : "Touch";
    ++revision;

    const bool isHover =
        (lastTouchReason == "Hover") ||
        (lastTouchReason.rfind("Hover", 0) == 0);

    char buf[256];
    std::snprintf(buf, sizeof(buf), "rev=%llu reason=%s",
        (unsigned long long)revision,
        lastTouchReason.c_str());

    EntityCore::EmitLog(EntityCore::LogLevel::Trace, "EntityBook",
        isHover ? "TouchHover" : "Touch",
        buf);
}

bool EntityBook::Initialize()
{
    entities.clear();

    try
    {
        entities.reserve(m_capacity);
    }
    catch (const std::bad_alloc&)
    {
        EntityCore::EmitLog(
            EntityCore::LogLevel::Error,
            "EntityBook",
            "Initialize",
            "Reserve failed: out of memory");

        return false;
    }
    catch (...)
    {
        EntityCore::EmitLog(
            EntityCore::LogLevel::Error,
            "EntityBook",
            "Initialize",
            "Reserve failed: unknown exception");

        return false;
    }

    const std::size_t reserved = entities.capacity();

    char buf[128];
    std::snprintf(buf, sizeof(buf),
        "Initialize | requested=%zu reserved=%zu",
        m_capacity, reserved);

    EntityCore::EmitLog(
        EntityCore::LogLevel::Info,
        "EntityBook",
        "Initialize",
        buf);

    if (reserved < m_capacity)
    {
        EntityCore::EmitLog(
            EntityCore::LogLevel::Error,
            "EntityBook",
            "Initialize",
            "Reserve did not meet requested capacity");

        return false;
    }

    return true;
}

void EntityBook::ClearEntityBook()
{
    Clear();
}
