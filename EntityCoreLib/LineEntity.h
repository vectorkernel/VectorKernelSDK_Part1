#pragma once
#include <memory>
#include <glm/glm.hpp>
#include "Entity.h"

class LineEntity : public Entity
{
public:
    union { glm::vec3 p0; glm::vec3 start; };
    union { glm::vec3 p1; glm::vec3 end; };
    glm::vec4 color{ 1.0f };
    union { float width; float thickness; };

    LineEntity()
        : Entity(EntityType::Line)
        , p0(0.0f)
        , p1(0.0f)
        , color(1.0f)
        , width(1.0f)
    {
    }

    std::unique_ptr<Entity> Clone() const override
    {
        return std::make_unique<LineEntity>(*this);
    }
};
