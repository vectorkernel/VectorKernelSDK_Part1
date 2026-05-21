#pragma once
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "Entity.h"

struct hershey_font;

enum class TextHAlign
{
    Left,
    Center,
    Right
};

class TextEntity : public Entity
{
public:
    std::string text;
    glm::vec3 position{ 0.0f };
    float boxWidth = 0.0f;
    float boxHeight = 0.0f;
    bool wordWrapEnabled = true;
    hershey_font* font = nullptr;
    union { TextHAlign hAlign; TextHAlign align; };
    float scale = 1.0f;
    float strokeWidth = 1.0f;
    glm::vec4 color{ 1.0f };
    float size = 1.0f;
    bool backgroundEnabled = false;
    glm::vec4 backgroundColor{ 0.0f, 0.0f, 0.0f, 1.0f };
    float backgroundPadding = 6.0f;
    bool backgroundOutlineEnabled = false;
    glm::vec4 backgroundOutlineColor{ 1.0f, 1.0f, 1.0f, 1.0f };
    float backgroundOutlineThickness = 1.0f;

    TextEntity()
        : Entity(EntityType::Text)
        , hAlign(TextHAlign::Left)
    {
    }

    std::unique_ptr<Entity> Clone() const override
    {
        return std::make_unique<TextEntity>(*this);
    }
};
