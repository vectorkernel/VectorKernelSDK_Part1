#pragma once
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Entity.h"

class PolylineEntity : public Entity
{
public:
    std::vector<glm::vec3> vertices{};
    glm::vec4 color{ 1.0f };
    float width = 1.0f;
    bool closed = false;
    std::string linetype = "Continuous";

    PolylineEntity()
        : Entity(EntityType::Polyline)
    {
    }

    std::unique_ptr<Entity> Clone() const override
    {
        return std::make_unique<PolylineEntity>(*this);
    }
};
