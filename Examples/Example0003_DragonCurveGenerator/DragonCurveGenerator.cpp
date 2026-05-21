#include "DragonCurveGenerator.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <utility>

namespace
{
    using Clock = std::chrono::system_clock;

    enum class CardinalDirection
    {
        North,
        East,
        South,
        West
    };

    DragonCurveGenerator::Point AdvancePoint(DragonCurveGenerator::Point point, CardinalDirection direction)
    {
        switch (direction)
        {
        case CardinalDirection::North:
            point.y -= 1;
            break;
        case CardinalDirection::East:
            point.x += 1;
            break;
        case CardinalDirection::South:
            point.y += 1;
            break;
        case CardinalDirection::West:
            point.x -= 1;
            break;
        }

        return point;
    }

    CardinalDirection Turn(CardinalDirection direction, int turn)
    {
        const bool turnRight = (turn > 0);

        switch (direction)
        {
        case CardinalDirection::North:
            return turnRight ? CardinalDirection::East : CardinalDirection::West;
        case CardinalDirection::East:
            return turnRight ? CardinalDirection::South : CardinalDirection::North;
        case CardinalDirection::South:
            return turnRight ? CardinalDirection::West : CardinalDirection::East;
        case CardinalDirection::West:
            return turnRight ? CardinalDirection::North : CardinalDirection::South;
        }

        return direction;
    }

    std::string FormatTimePoint(const Clock::time_point& tp)
    {
        const std::time_t tt = Clock::to_time_t(tp);
        std::tm localTm{};
#if defined(_WIN32)
        localtime_s(&localTm, &tt);
#else
        localtime_r(&tt, &localTm);
#endif

        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(&localTm, "%Y-%m-%d %H:%M:%S")
            << '.' << std::setw(3) << std::setfill('0') << ms.count();
        return oss.str();
    }

    std::string ThreadIdToString(const std::thread::id& id)
    {
        std::ostringstream oss;
        oss << id;
        return oss.str();
    }

    class ConsoleEventLog
    {
    public:
        void Start(const DragonCurveGenerator::ThreadTelemetry& telemetry)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::cout << '[' << telemetry.startedAt << "] "
                      << telemetry.tag << " started | thread=" << telemetry.threadId
                      << " | range=[" << telemetry.rangeBegin << ',' << telemetry.rangeEnd << ")"
                      << " | activity=" << telemetry.activity << '\n';
        }

        void Finish(const DragonCurveGenerator::ThreadTelemetry& telemetry)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::cout << '[' << telemetry.finishedAt << "] "
                      << telemetry.tag << " done    | thread=" << telemetry.threadId
                      << " | units=" << telemetry.unitsCompleted
                      << " | elapsed=" << telemetry.elapsedMilliseconds << " ms\n";
        }

    private:
        std::mutex m_mutex;
    };
}

DragonCurveGenerator::DragonCurveGenerator(int order)
{
    SetOrder(order);
}

void DragonCurveGenerator::SetOrder(int order)
{
    if (order < 0)
    {
        throw std::invalid_argument("DragonCurveGenerator order must be >= 0.");
    }

    m_order = order;
}

int DragonCurveGenerator::GetOrder() const noexcept
{
    return m_order;
}

void DragonCurveGenerator::Generate()
{
    m_turns = BuildTurns(m_order);
    m_points = BuildPoints(m_turns);
    m_threadTelemetry.clear();
    m_segments = BuildSegments(m_points);
    m_threadCountUsed = 1;
}

void DragonCurveGenerator::GenerateThreaded(unsigned int requestedThreadCount)
{
    m_turns = BuildTurns(m_order);
    m_points = BuildPoints(m_turns);
    m_threadTelemetry.clear();
    m_segments = BuildSegmentsThreaded(m_points, requestedThreadCount, m_threadTelemetry, m_threadCountUsed);
}

const std::vector<int>& DragonCurveGenerator::GetTurns() const noexcept
{
    return m_turns;
}

const std::vector<DragonCurveGenerator::Point>& DragonCurveGenerator::GetPoints() const noexcept
{
    return m_points;
}

const std::vector<DragonCurveGenerator::Segment>& DragonCurveGenerator::GetSegments() const noexcept
{
    return m_segments;
}

const std::vector<DragonCurveGenerator::ThreadTelemetry>& DragonCurveGenerator::GetThreadTelemetry() const noexcept
{
    return m_threadTelemetry;
}

std::size_t DragonCurveGenerator::GetPointCount() const noexcept
{
    return m_points.size();
}

std::size_t DragonCurveGenerator::GetSegmentCount() const noexcept
{
    return m_segments.size();
}

unsigned int DragonCurveGenerator::GetThreadCountUsed() const noexcept
{
    return m_threadCountUsed;
}

std::vector<int> DragonCurveGenerator::BuildTurns(int order)
{
    std::vector<int> turns;
    if (order <= 0)
    {
        return turns;
    }

    turns.reserve((1u << order) - 1u);

    for (int iteration = 0; iteration < order; ++iteration)
    {
        std::vector<int> next = turns;
        next.reserve((turns.size() * 2u) + 1u);
        next.push_back(1);

        for (auto it = turns.rbegin(); it != turns.rend(); ++it)
        {
            next.push_back(-*it);
        }

        turns.swap(next);
    }

    return turns;
}

std::vector<DragonCurveGenerator::Point> DragonCurveGenerator::BuildPoints(const std::vector<int>& turns)
{
    std::vector<Point> points;
    points.reserve(turns.size() + 2u);

    CardinalDirection direction = CardinalDirection::West;
    Point current{ 0, 0 };
    points.push_back(current);

    current = AdvancePoint(current, direction);
    points.push_back(current);

    for (const int turn : turns)
    {
        direction = Turn(direction, turn);
        current = AdvancePoint(current, direction);
        points.push_back(current);
    }

    return points;
}

std::vector<DragonCurveGenerator::Segment> DragonCurveGenerator::BuildSegments(const std::vector<Point>& points)
{
    std::vector<Segment> segments;
    if (points.size() < 2u)
    {
        return segments;
    }

    segments.reserve(points.size() - 1u);

    for (std::size_t i = 1; i < points.size(); ++i)
    {
        segments.push_back({ points[i - 1u], points[i] });
    }

    return segments;
}

std::vector<DragonCurveGenerator::Segment> DragonCurveGenerator::BuildSegmentsThreaded(
    const std::vector<Point>& points,
    unsigned int requestedThreadCount,
    std::vector<ThreadTelemetry>& telemetry,
    unsigned int& actualThreadCountUsed)
{
    std::vector<Segment> segments;
    if (points.size() < 2u)
    {
        actualThreadCountUsed = 1;
        telemetry.clear();
        return segments;
    }

    const std::size_t totalSegments = points.size() - 1u;
    const unsigned int hardwareThreads = std::max(1u, std::thread::hardware_concurrency());
    const unsigned int desiredThreads = requestedThreadCount == 0 ? hardwareThreads : requestedThreadCount;
    const unsigned int workerCount = std::max(1u, std::min<unsigned int>(desiredThreads, static_cast<unsigned int>(totalSegments)));

    actualThreadCountUsed = workerCount;
    segments.resize(totalSegments);
    telemetry.assign(workerCount, ThreadTelemetry{});

    ConsoleEventLog eventLog;
    std::vector<std::thread> workers;
    workers.reserve(workerCount);

    const std::size_t segmentsPerWorker = (totalSegments + static_cast<std::size_t>(workerCount) - 1u) / static_cast<std::size_t>(workerCount);

    for (unsigned int workerIndex = 0; workerIndex < workerCount; ++workerIndex)
    {
        const std::size_t begin = static_cast<std::size_t>(workerIndex) * segmentsPerWorker;
        const std::size_t end = std::min(totalSegments, begin + segmentsPerWorker);

        workers.emplace_back([&, workerIndex, begin, end]()
        {
            ThreadTelemetry local;
            local.tag = "[T" + std::to_string(workerIndex) + "]";
            local.threadId = ThreadIdToString(std::this_thread::get_id());
            local.activity = "build segment slice";
            local.rangeBegin = begin;
            local.rangeEnd = end;

            const auto startTime = Clock::now();
            local.startedAt = FormatTimePoint(startTime);
            eventLog.Start(local);

            for (std::size_t i = begin; i < end; ++i)
            {
                segments[i] = Segment{ points[i], points[i + 1u] };
                ++local.unitsCompleted;
            }

            const auto finishTime = Clock::now();
            local.finishedAt = FormatTimePoint(finishTime);
            local.elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(finishTime - startTime).count();
            telemetry[workerIndex] = local;
            eventLog.Finish(local);
        });
    }

    for (auto& worker : workers)
    {
        worker.join();
    }

    return segments;
}
