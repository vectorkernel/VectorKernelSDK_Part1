#include "pch.h"

#include "DragonGeneration.h"

#include <algorithm>
#include <future>
#include <thread>
#include <utility>
#include <vector>

namespace RenderCore
{
    namespace
    {
        glm::vec2 AdvancePoint(glm::vec2 point, DragonCardinalDir direction, float step)
        {
            switch (direction)
            {
            case DragonCardinalDir::North: point.y -= step; break;
            case DragonCardinalDir::East:  point.x += step; break;
            case DragonCardinalDir::South: point.y += step; break;
            case DragonCardinalDir::West:  point.x -= step; break;
            }
            return point;
        }

        struct PassBuildOutput
        {
            std::vector<glm::vec2> points;
            glm::vec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
            glm::vec2 minPt{ 0.0f, 0.0f };
            glm::vec2 maxPt{ 0.0f, 0.0f };
            bool hasBounds = false;
        };
    }

    std::vector<int> BuildDragonTurns(int order)
    {
        std::vector<int> turns;
        if (order <= 0)
            return turns;

        turns.reserve((1u << order) - 1u);

        for (int iteration = 0; iteration < order; ++iteration)
        {
            std::vector<int> next = turns;
            next.reserve((turns.size() * 2u) + 1u);
            next.push_back(1);

            for (auto it = turns.rbegin(); it != turns.rend(); ++it)
                next.push_back(-*it);

            turns.swap(next);
        }

        return turns;
    }

    DragonCardinalDir AdvanceDragonDir(DragonCardinalDir dir, int turn)
    {
        const bool turnRight = (turn > 0);

        switch (dir)
        {
        case DragonCardinalDir::North: return turnRight ? DragonCardinalDir::East : DragonCardinalDir::West;
        case DragonCardinalDir::East:  return turnRight ? DragonCardinalDir::South : DragonCardinalDir::North;
        case DragonCardinalDir::South: return turnRight ? DragonCardinalDir::West : DragonCardinalDir::East;
        case DragonCardinalDir::West:  return turnRight ? DragonCardinalDir::North : DragonCardinalDir::South;
        }

        return dir;
    }

    DragonCardinalDir ComputeDragonEndDir(const std::vector<int>& turns, DragonCardinalDir startDir)
    {
        DragonCardinalDir dir = startDir;

        if (!turns.empty())
        {
            dir = AdvanceDragonDir(dir, 1);
            for (const int turn : turns)
                dir = AdvanceDragonDir(dir, turn);
        }

        return dir;
    }

    std::vector<glm::vec2> BuildDragonPassPoints(const std::vector<int>& turns,
                                                 const glm::ivec2& startOffset,
                                                 DragonCardinalDir startDir,
                                                 float step)
    {
        std::vector<glm::vec2> points;
        points.reserve(turns.size() + 2u);

        glm::vec2 current(static_cast<float>(startOffset.x) * step,
                          static_cast<float>(startOffset.y) * step);
        points.push_back(current);

        DragonCardinalDir direction = startDir;
        current = AdvancePoint(current, direction, step);
        points.push_back(current);

        for (const int turn : turns)
        {
            direction = AdvanceDragonDir(direction, turn);
            current = AdvancePoint(current, direction, step);
            points.push_back(current);
        }

        return points;
    }

    DragonBuildResult BuildDragonGeometry(const DragonBuildRequest& request)
    {
        DragonBuildResult result;

        if (request.order <= 0 || request.step <= 0.0f || request.passes.empty())
            return result;

        const std::vector<int> turns = BuildDragonTurns(request.order);
        if (turns.empty())
            return result;

        const unsigned int hardwareThreads = std::max(1u, std::thread::hardware_concurrency());
        const unsigned int requestedThreads = request.threadCount == 0 ? hardwareThreads : request.threadCount;
        const unsigned int maxThreads = std::max(1u, requestedThreads);

        std::vector<PassBuildOutput> builtPasses(request.passes.size());

        auto buildOnePass = [&turns, step = request.step](const DragonPassSpec& pass) -> PassBuildOutput
        {
            PassBuildOutput out;
            out.points = BuildDragonPassPoints(turns, pass.startOffset, pass.startDir, step);
            out.color = pass.color;

            for (const glm::vec2& pt : out.points)
            {
                if (!out.hasBounds)
                {
                    out.minPt = pt;
                    out.maxPt = pt;
                    out.hasBounds = true;
                }
                else
                {
                    out.minPt = glm::min(out.minPt, pt);
                    out.maxPt = glm::max(out.maxPt, pt);
                }
            }

            return out;
        };

        const unsigned int passWorkerCount = std::min<unsigned int>(maxThreads, static_cast<unsigned int>(request.passes.size()));
        if (passWorkerCount <= 1u)
        {
            for (std::size_t i = 0; i < request.passes.size(); ++i)
                builtPasses[i] = buildOnePass(request.passes[i]);
        }
        else
        {
            std::vector<std::future<PassBuildOutput>> futures;
            futures.reserve(request.passes.size());
            for (const DragonPassSpec& pass : request.passes)
                futures.push_back(std::async(std::launch::async, buildOnePass, pass));
            for (std::size_t i = 0; i < futures.size(); ++i)
                builtPasses[i] = futures[i].get();
        }

        std::vector<std::size_t> passSegmentOffsets(builtPasses.size(), 0u);
        std::size_t totalSegments = 0u;

        for (std::size_t i = 0; i < builtPasses.size(); ++i)
        {
            const PassBuildOutput& pass = builtPasses[i];
            if (!pass.hasBounds)
                continue;

            if (!result.hasGeometry)
            {
                result.minPt = pass.minPt;
                result.maxPt = pass.maxPt;
                result.hasGeometry = true;
            }
            else
            {
                result.minPt = glm::min(result.minPt, pass.minPt);
                result.maxPt = glm::max(result.maxPt, pass.maxPt);
            }

            result.pointCount += pass.points.size();
            passSegmentOffsets[i] = totalSegments;
            if (pass.points.size() > 1u)
                totalSegments += (pass.points.size() - 1u);
        }

        if (!result.hasGeometry || totalSegments == 0u)
            return result;

        result.center = 0.5f * (result.minPt + result.maxPt);
        result.segments.resize(totalSegments);

        struct SegmentChunk
        {
            std::size_t passIndex = 0u;
            std::size_t localBegin = 0u;
            std::size_t localEnd = 0u;
            std::size_t dstBegin = 0u;
        };

        const std::size_t chunkTarget = std::max<std::size_t>(1u, (totalSegments + static_cast<std::size_t>(maxThreads) - 1u) / static_cast<std::size_t>(maxThreads));
        std::vector<SegmentChunk> chunks;
        chunks.reserve(maxThreads * 2u);

        for (std::size_t passIndex = 0; passIndex < builtPasses.size(); ++passIndex)
        {
            const PassBuildOutput& pass = builtPasses[passIndex];
            const std::size_t passSegmentCount = pass.points.size() > 1u ? (pass.points.size() - 1u) : 0u;
            if (passSegmentCount == 0u)
                continue;

            for (std::size_t localBegin = 0u; localBegin < passSegmentCount; localBegin += chunkTarget)
            {
                const std::size_t localEnd = std::min(passSegmentCount, localBegin + chunkTarget);
                chunks.push_back(SegmentChunk{ passIndex, localBegin, localEnd, passSegmentOffsets[passIndex] + localBegin });
            }
        }

        auto writeChunkRange = [&builtPasses, &result, &chunks, centerResult = request.centerResult, center = result.center](std::size_t chunkBegin, std::size_t chunkEnd)
        {
            for (std::size_t chunkIndex = chunkBegin; chunkIndex < chunkEnd; ++chunkIndex)
            {
                const SegmentChunk& chunk = chunks[chunkIndex];
                const PassBuildOutput& pass = builtPasses[chunk.passIndex];

                for (std::size_t i = chunk.localBegin; i < chunk.localEnd; ++i)
                {
                    DragonSegment segment;
                    segment.a = pass.points[i];
                    segment.b = pass.points[i + 1u];
                    if (centerResult)
                    {
                        segment.a -= center;
                        segment.b -= center;
                    }
                    segment.color = pass.color;
                    result.segments[chunk.dstBegin + (i - chunk.localBegin)] = segment;
                }
            }
        };

        const unsigned int chunkWorkerCount = std::min<unsigned int>(maxThreads, static_cast<unsigned int>(chunks.size()));
        result.workerCountUsed = std::max(1u, std::max(passWorkerCount, chunkWorkerCount));

        if (chunkWorkerCount <= 1u)
        {
            writeChunkRange(0u, chunks.size());
        }
        else
        {
            std::vector<std::future<void>> futures;
            futures.reserve(chunkWorkerCount);
            const std::size_t chunksPerWorker = std::max<std::size_t>(1u, (chunks.size() + static_cast<std::size_t>(chunkWorkerCount) - 1u) / static_cast<std::size_t>(chunkWorkerCount));

            for (unsigned int workerIndex = 0; workerIndex < chunkWorkerCount; ++workerIndex)
            {
                const std::size_t chunkBegin = static_cast<std::size_t>(workerIndex) * chunksPerWorker;
                if (chunkBegin >= chunks.size())
                    break;

                const std::size_t chunkEnd = std::min(chunks.size(), chunkBegin + chunksPerWorker);
                futures.push_back(std::async(std::launch::async, writeChunkRange, chunkBegin, chunkEnd));
            }

            for (auto& future : futures)
                future.get();
        }

        if (request.centerResult)
        {
            result.minPt -= result.center;
            result.maxPt -= result.center;
            result.center = glm::vec2(0.0f, 0.0f);
        }

        return result;
    }
}
