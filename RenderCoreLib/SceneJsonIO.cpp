// SceneJsonIO.cpp
#include "pch.h"
#include "SceneJsonIO.h"

#include <fstream>
#include <json/json.h>

#include <glm/glm.hpp>

#include "LineEntity.h"
#include "TextEntity.h"
#include "SolidRectEntity.h"

static Json::Value Vec3ToJson(const glm::vec3& v)
{
    Json::Value a(Json::arrayValue);
    a.append(v.x);
    a.append(v.y);
    a.append(v.z);
    return a;
}

static Json::Value Vec4ToJson(const glm::vec4& v)
{
    Json::Value a(Json::arrayValue);
    a.append(v.x);
    a.append(v.y);
    a.append(v.z);
    a.append(v.w);
    return a;
}

static const char* EntityTypeToString(EntityType t)
{
    switch (t)
    {
    case EntityType::Line:      return "Line";
    case EntityType::Text:      return "Text";
    case EntityType::SolidRect: return "SolidRect";
    default:                    return "Unknown";
    }
}

static const char* EntityTagToString(EntityTag t)
{
    switch (t)
    {
    case EntityTag::Grid:   return "Grid";
    case EntityTag::Scene:  return "Scene";
    case EntityTag::Cursor: return "Cursor";
    case EntityTag::Hud:    return "Hud";
    default:                return "Unknown";
    }
}

static const char* TextHAlignToString(TextHAlign a)
{
    switch (a)
    {
    case TextHAlign::Left:   return "Left";
    case TextHAlign::Center: return "Center";
    case TextHAlign::Right:  return "Right";
    default:                 return "Left";
    }
}

bool SceneJsonIO::SaveEntitiesToFile(const std::string& filename, const EntityBook::EntityList& entities, int formatVersion)
{
    Json::Value root(Json::objectValue);
    root["format"] = "VectorKernel.Entities";
    root["version"] = formatVersion;

    Json::Value arr(Json::arrayValue);

    for (const auto& ep : entities)
    {
        if (!ep) continue;
        const Entity& e = *ep;
        Json::Value je(Json::objectValue);

        // Common fields
        je["id"] = (Json::UInt64)e.ID;
        je["type"] = EntityTypeToString(e.type);
        je["tag"] = EntityTagToString(e.tag);
        je["screenSpace"] = e.screenSpace;
        je["drawOrder"] = e.drawOrder;

        je["layerId"] = e.layerId;
        je["colorByLayer"] = e.colorByLayer;
        je["linetypeByLayer"] = e.linetypeByLayer;
        je["linetypeOverride"] = e.linetypeOverride;

        // Payload by type
        switch (e.type)
        {
        case EntityType::Line:
            { const auto& line = static_cast<const LineEntity&>(e); je["line"]["p0"] = Vec3ToJson(line.p0); je["line"]["p1"] = Vec3ToJson(line.p1); je["line"]["color"] = Vec4ToJson(line.color); je["line"]["width"] = line.width; }
            
            break;

        case EntityType::Text:
            { const auto& text = static_cast<const TextEntity&>(e); je["text"]["text"] = text.text; je["text"]["position"] = Vec3ToJson(text.position); je["text"]["boxWidth"] = text.boxWidth; je["text"]["boxHeight"] = text.boxHeight; je["text"]["wordWrapEnabled"] = text.wordWrapEnabled; je["text"]["hAlign"] = TextHAlignToString(text.hAlign); je["text"]["scale"] = text.scale; je["text"]["strokeWidth"] = text.strokeWidth; je["text"]["color"] = Vec4ToJson(text.color); je["text"]["size"] = text.size; je["text"]["backgroundEnabled"] = text.backgroundEnabled; je["text"]["backgroundColor"] = Vec4ToJson(text.backgroundColor); je["text"]["backgroundPadding"] = text.backgroundPadding; }
            
            break;

        case EntityType::SolidRect:
            { const auto& rect = static_cast<const SolidRectEntity&>(e); je["solidRect"]["min"] = Vec3ToJson(rect.min); je["solidRect"]["max"] = Vec3ToJson(rect.max); je["solidRect"]["color"] = Vec4ToJson(rect.color); }
            
            break;
        }

        arr.append(je);
    }

    root["entities"] = arr;

    Json::StreamWriterBuilder b;
    b["indentation"] = "  "; // pretty

    std::ofstream out(filename, std::ios::out | std::ios::trunc);
    if (!out.is_open())
        return false;

    std::unique_ptr<Json::StreamWriter> w(b.newStreamWriter());
    w->write(root, &out);
    out << "\n";
    return true;
}
