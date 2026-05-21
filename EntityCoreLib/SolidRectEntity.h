#pragma once
#include <memory>
#include <glm/glm.hpp>
#include "Entity.h"

class SolidRectEntity : public Entity
{
public:
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
    glm::vec4 color{0.0f, 0.0f, 0.0f, 1.0f};

    SolidRectEntity()
        : Entity(EntityType::SolidRect)
    {
    }

    std::unique_ptr<Entity> Clone() const override
    {
        return std::make_unique<SolidRectEntity>(*this);
    }
};
