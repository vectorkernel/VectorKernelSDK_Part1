#include "DragonCurveGenerator.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>

namespace
{
    int ReadIntOrDefault(const char* text, int fallback)
    {
        if (text == nullptr)
        {
            return fallback;
        }

        try
        {
            return std::stoi(text);
        }
        catch (...)
        {
            return fallback;
        }
    }

    void PrintSegments(const DragonCurveGenerator& generator, std::size_t maxToPrint)
    {
        const auto& segments = generator.GetSegments();
        const std::size_t count = std::min(maxToPrint, segments.size());

        for (std::size_t i = 0; i < count; ++i)
        {
            const auto& segment = segments[i];
            std::cout
                << "segment[" << i << "] : ("
                << segment.start.x << ", " << segment.start.y << ") -> ("
                << segment.end.x << ", " << segment.end.y << ")\n";
        }

        if (segments.size() > count)
        {
            std::cout << "... " << (segments.size() - count) << " more segments not shown.\n";
        }
    }

    void PrintThreadSummary(const DragonCurveGenerator& generator)
    {
        const auto& telemetry = generator.GetThreadTelemetry();
        if (telemetry.empty())
        {
            std::cout << "Thread summary: single-threaded path used.\n";
            return;
        }

        std::cout << "\nWorker summary\n";
        std::cout << "--------------\n";
        std::cout << std::left
                  << std::setw(8)  << "Tag"
                  << std::setw(20) << "ThreadId"
                  << std::setw(16) << "Range"
                  << std::setw(10) << "Units"
                  << std::setw(12) << "Elapsed"
                  << "Activity\n";

        for (const auto& t : telemetry)
        {
            std::string range = std::to_string(t.rangeBegin) + "-" + std::to_string(t.rangeEnd);
            std::string elapsed = std::to_string(t.elapsedMilliseconds) + " ms";
            std::cout << std::left
                      << std::setw(8)  << t.tag
                      << std::setw(20) << t.threadId
                      << std::setw(16) << range
                      << std::setw(10) << t.unitsCompleted
                      << std::setw(12) << elapsed
                      << t.activity << '\n';
        }

        const auto totalUnits = std::accumulate(telemetry.begin(), telemetry.end(), std::size_t{ 0 },
            [](std::size_t sum, const DragonCurveGenerator::ThreadTelemetry& t)
            {
                return sum + t.unitsCompleted;
            });

        std::cout << "\nTotal units completed across workers: " << totalUnits << '\n';
    }
}

int main(int argc, char* argv[])
{
    try
    {
        const int order = std::max(0, ReadIntOrDefault(argc > 1 ? argv[1] : nullptr, 18));
        const int requestedThreadsRaw = std::max(0, ReadIntOrDefault(argc > 2 ? argv[2] : nullptr, static_cast<int>(std::thread::hardware_concurrency())));
        const unsigned int requestedThreads = static_cast<unsigned int>(requestedThreadsRaw);

        DragonCurveGenerator generator;
        generator.SetOrder(order);

        const auto generateStart = std::chrono::steady_clock::now();
        generator.GenerateThreaded(requestedThreads);
        const auto generateFinish = std::chrono::steady_clock::now();
        const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(generateFinish - generateStart).count();

        std::cout << "Example0003 - DragonCurveGenerator threaded console prototype\n";
        std::cout << "========================================================\n";
        std::cout << "Order:                " << generator.GetOrder() << "\n";
        std::cout << "Requested threads:    " << requestedThreads << "\n";
        std::cout << "Worker threads used:  " << generator.GetThreadCountUsed() << "\n";
        std::cout << "Turn count:           " << generator.GetTurns().size() << "\n";
        std::cout << "Point count:          " << generator.GetPointCount() << "\n";
        std::cout << "Segment count:        " << generator.GetSegmentCount() << "\n";
        std::cout << "Total generate time:  " << totalMs << " ms\n\n";

        PrintThreadSummary(generator);
        std::cout << '\n';
        PrintSegments(generator, 12);

        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
}
