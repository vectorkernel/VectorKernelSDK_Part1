#pragma once
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>

#include "Entity.h"

namespace EntityCore
{
    enum class LogLevel : uint8_t
    {
        Trace,
        Info,
        Warn,
        Error
    };

    using LogSink = void(*)(LogLevel level, const char* category, const char* event, std::string_view message);

    void SetLogSink(LogSink sink) noexcept;
    LogSink GetLogSink() noexcept;

    inline const char* ToString(EntityType t)
    {
        switch (t)
        {
        case EntityType::Line:      return "Line";
        case EntityType::Polyline:  return "Polyline";
        case EntityType::Bezier:    return "Bezier";
        case EntityType::Circle:    return "Circle";
        case EntityType::Polygon:   return "Polygon";
        case EntityType::Text:      return "Text";
        case EntityType::SolidRect: return "SolidRect";
        case EntityType::Block:     return "Block";
        default:                    return "Unknown";
        }
    }

    inline const char* ToString(EntityTag t)
    {
        switch (t)
        {
        case EntityTag::Grid:   return "Grid";
        case EntityTag::Scene:  return "Scene";
        case EntityTag::User:   return "User";
        case EntityTag::Cursor: return "Cursor";
        case EntityTag::Hud:    return "Hud";
        default:                return "Unknown";
        }
    }

    inline std::string DescribeEntity(const Entity& e)
    {
        std::ostringstream os;
        os << "id=" << e.ID
            << " type=" << ToString(e.type)
            << " tag=" << ToString(e.tag)
            << " drawOrder=" << e.drawOrder
            << " layerId=" << e.layerId
            << " screenSpace=" << (e.screenSpace ? "true" : "false")
            << " selected=" << (e.selected ? "true" : "false");
        return os.str();
    }

    inline void EmitLog(LogLevel level, const char* category, const char* event, std::string_view message)
    {
        if (auto* sink = GetLogSink())
            sink(level, category, event, message);
    }
}
