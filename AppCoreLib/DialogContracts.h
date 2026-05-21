#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

class EntityBook;
class LineEntity;
class LayerTable;
class SelectionController;

enum class EntityKind
{
    Unknown = 0,
    Line,
    Circle,
    Polyline
};

struct SelectedEntityInfo
{
    std::size_t entityId = 0;
    EntityKind kind = EntityKind::Unknown;
    std::string displayName;
    std::string layerName;
    std::string colorName;
};

struct SingleLineSelectionInfo
{
    std::size_t entityId = 0;
    bool screenSpace = false;
    float startX = 0.0f;
    float startY = 0.0f;
    float startZ = 0.0f;
    float endX = 0.0f;
    float endY = 0.0f;
    float endZ = 0.0f;
};

class IPropertiesHost
{
public:
    virtual ~IPropertiesHost() = default;

    virtual std::vector<SelectedEntityInfo> BuildSelectedEntities() const = 0;
    virtual bool TryGetSingleSelectedLine(SingleLineSelectionInfo& outInfo) const = 0;
    virtual bool UpdateSingleSelectedLine(const SingleLineSelectionInfo& info) = 0;
    virtual std::vector<std::pair<std::string, std::uint32_t>> BuildLayerChoices() const = 0;
    virtual bool AssignSelectionLayer(std::uint32_t layerId, EntityKind filterKind) = 0;
};
