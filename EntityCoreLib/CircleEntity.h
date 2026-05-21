#pragma once
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "Entity.h"

class CircleEntity : public Entity
{
public:
    glm::vec3 center{ 0.0f };
    float radius = 0.5f;
    glm::vec4 color{ 1.0f };
    float width = 1.0f;
    std::string linetype = "Continuous";
    bool filled = false;

    CircleEntity()
        : Entity(EntityType::Circle)
    {
    }

    std::unique_ptr<Entity> Clone() const override
    {
        return std::make_unique<CircleEntity>(*this);
    }
};
