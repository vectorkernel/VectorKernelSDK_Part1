#pragma once
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Entity.h"

class BlockEntity : public Entity
{
public:
    std::string name{};
    std::vector<std::size_t> childEntityIds{};
    glm::vec3 insertionPoint{ 0.0f };
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
    float rotationRadians = 0.0f;
    bool exploded = false;

    BlockEntity()
        : Entity(EntityType::Block)
    {
    }

    std::unique_ptr<Entity> Clone() const override
    {
        return std::make_unique<BlockEntity>(*this);
    }
};
