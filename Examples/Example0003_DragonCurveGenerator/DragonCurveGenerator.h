#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

class DragonCurveGenerator
{
public:
    struct Point
    {
        int x = 0;
        int y = 0;

        friend bool operator==(const Point&, const Point&) = default;
    };

    struct Segment
    {
        Point start;
        Point end;

        friend bool operator==(const Segment&, const Segment&) = default;
    };

    struct ThreadTelemetry
    {
        std::string tag;
        std::string threadId;
        std::string activity;
        std::string startedAt;
        std::string finishedAt;
        std::size_t rangeBegin = 0;
        std::size_t rangeEnd = 0;
        std::size_t unitsCompleted = 0;
        long long elapsedMilliseconds = 0;
    };

public:
    DragonCurveGenerator() = default;
    explicit DragonCurveGenerator(int order);

    void SetOrder(int order);
    int GetOrder() const noexcept;

    void Generate();
    void GenerateThreaded(unsigned int requestedThreadCount = 0);

    const std::vector<int>& GetTurns() const noexcept;
    const std::vector<Point>& GetPoints() const noexcept;
    const std::vector<Segment>& GetSegments() const noexcept;
    const std::vector<ThreadTelemetry>& GetThreadTelemetry() const noexcept;

    std::size_t GetPointCount() const noexcept;
    std::size_t GetSegmentCount() const noexcept;
    unsigned int GetThreadCountUsed() const noexcept;

private:
    static std::vector<int> BuildTurns(int order);
    static std::vector<Point> BuildPoints(const std::vector<int>& turns);
    static std::vector<Segment> BuildSegments(const std::vector<Point>& points);
    static std::vector<Segment> BuildSegmentsThreaded(const std::vector<Point>& points,
                                                      unsigned int requestedThreadCount,
                                                      std::vector<ThreadTelemetry>& telemetry,
                                                      unsigned int& actualThreadCountUsed);

private:
    int m_order = 0;
    unsigned int m_threadCountUsed = 1;
    std::vector<int> m_turns;
    std::vector<Point> m_points;
    std::vector<Segment> m_segments;
    std::vector<ThreadTelemetry> m_threadTelemetry;
};
