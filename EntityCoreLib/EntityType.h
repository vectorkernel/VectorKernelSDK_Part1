// EntityType.h
#pragma once

enum class EntityType
{
    Line,
    Polyline,
    Bezier,
    Circle,
    Polygon,
    Text,
    SolidRect,
    Block
};


enum class EntityTag
{
    Grid,   // background
    Scene,  // main world entities
    User,   // user/script-created entities (persist across scene rebuilds)
    Cursor, // overlay cursor/picker
    Hud,    // top overlay
    Paper,   // paper-space chrome (page borders, viewport frames)
    PaperUser // user-created paper-space annotation entities
};
