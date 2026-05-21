#pragma once
#include <array>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "Entity.h"

class BezierEntity : public Entity
{
public:
    std::array<glm::vec3, 4> controlPoints{
        glm::vec3{ 0.0f },
        glm::vec3{ 0.0f },
        glm::vec3{ 0.0f },
        glm::vec3{ 0.0f }
    };
    glm::vec4 color{ 1.0f };
    float width = 1.0f;
    std::string linetype = "Continuous";

    BezierEntity()
        : Entity(EntityType::Bezier)
    {
    }

    std::unique_ptr<Entity> Clone() const override
    {
        return std::make_unique<BezierEntity>(*this);
    }
};
