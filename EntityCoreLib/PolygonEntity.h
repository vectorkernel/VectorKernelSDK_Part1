#pragma once
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Entity.h"

class PolygonEntity : public Entity
{
public:
    std::vector<glm::vec3> vertices{};
    glm::vec4 color{ 1.0f };
    float width = 1.0f;
    std::string linetype = "Continuous";
    bool filled = false;

    PolygonEntity()
        : Entity(EntityType::Polygon)
    {
    }

    std::unique_ptr<Entity> Clone() const override
    {
        return std::make_unique<PolygonEntity>(*this);
    }
};
