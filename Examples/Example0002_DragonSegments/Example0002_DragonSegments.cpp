#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr int kDefaultLevel = 17;
    constexpr int kSegmentLength = 1;

    struct ColorInfo
    {
        const char* name;
        std::uint32_t value;
    };

    constexpr std::array<ColorInfo, 4> kPassColors = {{
        {"Blue",   0x0000FFu},
        {"Green",  0x00FF00u},
        {"Red",    0xFF0000u},
        {"Yellow", 0x00FFFFu}
    }};

    constexpr std::array<int, 8> kPassStartOffsets = { 0, 0, 1, -1, 1, -1, 1, 0 };

    enum class Direction
    {
        North,
        East,
        South,
        West
    };

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
        int passIndex = 0;
        std::size_t stepIndex = 0;
        const char* colorName = "";
        std::uint32_t colorValue = 0;
    };

    struct SegmentKey
    {
        int ax = 0;
        int ay = 0;
        int bx = 0;
        int by = 0;

        friend bool operator==(const SegmentKey&, const SegmentKey&) = default;
    };

    struct SegmentKeyHasher
    {
        std::size_t operator()(const SegmentKey& key) const noexcept
        {
            std::size_t seed = 0;
            auto mix = [&seed](int value)
            {
                seed ^= std::hash<int>{}(value) + 0x9e3779b9u + (seed << 6) + (seed >> 2);
            };

            mix(key.ax);
            mix(key.ay);
            mix(key.bx);
            mix(key.by);
            return seed;
        }
    };

    struct OverlapRecord
    {
        SegmentKey key;
        std::vector<std::size_t> indices;
    };

    std::string GenerateTurns(int level)
    {
        if (level <= 0)
        {
            return {};
        }

        std::string turns;
        turns.push_back(1);

        for (int i = 0; i < level - 1; ++i)
        {
            std::string next = turns;
            next.push_back(1);
            for (auto it = turns.rbegin(); it != turns.rend(); ++it)
            {
                next.push_back(*it == 0 ? 1 : 0);
            }
            turns = std::move(next);
        }

        return turns;
    }

    Point Advance(Point point, Direction direction)
    {
        switch (direction)
        {
        case Direction::North: point.y -= kSegmentLength; break;
        case Direction::East:  point.x += kSegmentLength; break;
        case Direction::South: point.y += kSegmentLength; break;
        case Direction::West:  point.x -= kSegmentLength; break;
        }
        return point;
    }

    Direction Turn(Direction direction, char turnCode)
    {
        const bool turnRight = (turnCode != 0);

        switch (direction)
        {
        case Direction::North: return turnRight ? Direction::East : Direction::West;
        case Direction::East:  return turnRight ? Direction::South : Direction::North;
        case Direction::South: return turnRight ? Direction::West : Direction::East;
        case Direction::West:  return turnRight ? Direction::North : Direction::South;
        }

        return direction;
    }

    std::vector<Segment> BuildSegmentsForPass(const std::string& turns, int passIndex)
    {
        std::vector<Segment> segments;
        segments.reserve(turns.size());

        Point current{ kPassStartOffsets[passIndex * 2], kPassStartOffsets[passIndex * 2 + 1] };
        Direction direction = Direction::West;

        for (std::size_t stepIndex = 0; stepIndex < turns.size(); ++stepIndex)
        {
            Point next = Advance(current, direction);
            segments.push_back({ current, next, passIndex, stepIndex, kPassColors[passIndex].name, kPassColors[passIndex].value });
            current = next;
            direction = Turn(direction, turns[stepIndex]);
        }

        return segments;
    }

    std::vector<Segment> BuildAllSegments(const std::string& turns)
    {
        std::vector<Segment> all;
        all.reserve(turns.size() * kPassColors.size());

        for (int passIndex = 0; passIndex < static_cast<int>(kPassColors.size()); ++passIndex)
        {
            auto passSegments = BuildSegmentsForPass(turns, passIndex);
            all.insert(all.end(), passSegments.begin(), passSegments.end());
        }

        return all;
    }

    SegmentKey Normalize(const Segment& segment)
    {
        const bool ordered =
            (segment.start.x < segment.end.x) ||
            (segment.start.x == segment.end.x && segment.start.y <= segment.end.y);

        if (ordered)
        {
            return { segment.start.x, segment.start.y, segment.end.x, segment.end.y };
        }

        return { segment.end.x, segment.end.y, segment.start.x, segment.start.y };
    }

    std::vector<OverlapRecord> FindOverlaps(const std::vector<Segment>& segments)
    {
        std::unordered_map<SegmentKey, std::vector<std::size_t>, SegmentKeyHasher> buckets;
        buckets.reserve(segments.size());

        for (std::size_t i = 0; i < segments.size(); ++i)
        {
            buckets[Normalize(segments[i])].push_back(i);
        }

        std::vector<OverlapRecord> overlaps;
        for (const auto& [key, indices] : buckets)
        {
            if (indices.size() > 1)
            {
                overlaps.push_back({ key, indices });
            }
        }

        std::sort(overlaps.begin(), overlaps.end(), [](const OverlapRecord& a, const OverlapRecord& b)
        {
            if (a.indices.size() != b.indices.size())
            {
                return a.indices.size() > b.indices.size();
            }
            if (a.key.ax != b.key.ax) return a.key.ax < b.key.ax;
            if (a.key.ay != b.key.ay) return a.key.ay < b.key.ay;
            if (a.key.bx != b.key.bx) return a.key.bx < b.key.bx;
            return a.key.by < b.key.by;
        });

        return overlaps;
    }

    int ReadLevel(int argc, char* argv[])
    {
        if (argc < 2)
        {
            return kDefaultLevel;
        }

        try
        {
            const int parsed = std::stoi(argv[1]);
            return parsed > 0 ? parsed : kDefaultLevel;
        }
        catch (...)
        {
            return kDefaultLevel;
        }
    }

    void PrintColorScheme()
    {
        std::cout << "Color scheme preserved from Example0001:\n";
        for (std::size_t i = 0; i < kPassColors.size(); ++i)
        {
            std::cout << "  Pass " << i
                      << ": " << kPassColors[i].name
                      << " (0x" << std::hex << std::uppercase << kPassColors[i].value << std::dec << ")\n";
        }
        std::cout << '\n';
    }

    void PrintSampleSegments(const std::vector<Segment>& segments, std::size_t count)
    {
        const std::size_t shown = std::min(count, segments.size());
        std::cout << "First " << shown << " segments:\n";
        for (std::size_t i = 0; i < shown; ++i)
        {
            const Segment& s = segments[i];
            std::cout << "  [" << i << "] "
                      << "pass=" << s.passIndex
                      << " color=" << s.colorName
                      << " step=" << s.stepIndex
                      << " start=(" << s.start.x << ',' << s.start.y << ')'
                      << " end=(" << s.end.x << ',' << s.end.y << ')'
                      << "\n";
        }
        std::cout << '\n';
    }

    void PrintOverlapSummary(const std::vector<OverlapRecord>& overlaps, const std::vector<Segment>& segments)
    {
        std::size_t repeatedSegments = 0;
        for (const auto& overlap : overlaps)
        {
            repeatedSegments += overlap.indices.size();
        }

        if (overlaps.empty())
        {
            std::cout << "Exact overlaps: none\n";
            return;
        }

        std::cout << "Exact overlap groups: " << overlaps.size() << '\n';
        std::cout << "Segments participating in overlaps: " << repeatedSegments << "\n\n";

        const std::size_t maxGroupsToPrint = std::min<std::size_t>(overlaps.size(), 10);
        std::cout << "First " << maxGroupsToPrint << " overlap groups:\n";
        for (std::size_t i = 0; i < maxGroupsToPrint; ++i)
        {
            const auto& overlap = overlaps[i];
            std::cout << "  Group " << i
                      << ": segment ("
                      << overlap.key.ax << ',' << overlap.key.ay << ") -> ("
                      << overlap.key.bx << ',' << overlap.key.by << ")"
                      << " repeated " << overlap.indices.size() << " times\n";

            for (std::size_t index : overlap.indices)
            {
                const Segment& s = segments[index];
                std::cout << "    globalIndex=" << index
                          << " pass=" << s.passIndex
                          << " color=" << s.colorName
                          << " step=" << s.stepIndex
                          << '\n';
            }
        }
    }
}

int main(int argc, char* argv[])
{
    const int level = ReadLevel(argc, argv);
    const std::string turns = GenerateTurns(level);
    const std::vector<Segment> segments = BuildAllSegments(turns);
    const std::vector<OverlapRecord> overlaps = FindOverlaps(segments);

    std::cout << "Example0002_DragonSegments\n";
    std::cout << "==========================\n\n";
    std::cout << "Requested dragon level: " << level << '\n';
    std::cout << "Turn instruction count: " << turns.size() << '\n';
    std::cout << "Total stored segments:  " << segments.size() << "\n\n";

    PrintColorScheme();
    PrintSampleSegments(segments, 12);
    PrintOverlapSummary(overlaps, segments);

    std::cout << "\nDone.\n";
    return 0;
}
