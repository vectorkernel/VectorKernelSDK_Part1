#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "EntityType.h"

class Entity
{
public:
    virtual ~Entity() = default;

    std::size_t ID = 0;
    EntityType type;
    EntityTag  tag = EntityTag::Scene;

    bool screenSpace = false;
    bool pickable = true;
    bool visible = true;
    bool selected = false;

    int drawOrder = 0;

    uint32_t layerId = 0;
    bool colorByLayer = true;
    bool linetypeByLayer = true;
    std::string linetypeOverride = "Continuous";

    virtual std::unique_ptr<Entity> Clone() const = 0;

    template <typename T>
    T* As() noexcept { return dynamic_cast<T*>(this); }

    template <typename T>
    const T* As() const noexcept { return dynamic_cast<const T*>(this); }

protected:
    explicit Entity(EntityType inType)
        : type(inType)
    {
    }
};
