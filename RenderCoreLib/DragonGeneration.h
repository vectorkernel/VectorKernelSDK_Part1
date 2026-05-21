#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

namespace RenderCore
{
    enum class DragonCardinalDir
    {
        North,
        East,
        South,
        West
    };

    struct DragonPassSpec
    {
        glm::ivec2 startOffset{ 0, 0 };
        DragonCardinalDir startDir = DragonCardinalDir::West;
        glm::vec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
    };

    struct DragonSegment
    {
        glm::vec2 a{ 0.0f, 0.0f };
        glm::vec2 b{ 0.0f, 0.0f };
        glm::vec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
    };

    struct DragonBuildRequest
    {
        int order = 15;
        float step = 6.0f;
        std::vector<DragonPassSpec> passes;
        unsigned int threadCount = 0;
        bool centerResult = true;
    };

    struct DragonBuildResult
    {
        std::vector<DragonSegment> segments;
        glm::vec2 minPt{ 0.0f, 0.0f };
        glm::vec2 maxPt{ 0.0f, 0.0f };
        glm::vec2 center{ 0.0f, 0.0f };
        std::size_t pointCount = 0;
        unsigned int workerCountUsed = 1;
        bool hasGeometry = false;
    };

    std::vector<int> BuildDragonTurns(int order);
    DragonCardinalDir AdvanceDragonDir(DragonCardinalDir dir, int turn);
    DragonCardinalDir ComputeDragonEndDir(const std::vector<int>& turns, DragonCardinalDir startDir);
    std::vector<glm::vec2> BuildDragonPassPoints(const std::vector<int>& turns,
                                                 const glm::ivec2& startOffset,
                                                 DragonCardinalDir startDir,
                                                 float step);

    DragonBuildResult BuildDragonGeometry(const DragonBuildRequest& request);
}
