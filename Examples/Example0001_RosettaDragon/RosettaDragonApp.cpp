#include "RosettaDragonApp.h"

#include "RosettaBitmap.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

namespace
{
    constexpr int kBitmapSize = 800;
    constexpr int kSegmentLength = 1;
    constexpr int kIterations = 17;

    enum Direction
    {
        North = 1,
        East = 2,
        South = 4,
        West = 8
    };

    class RosettaDragon
    {
    public:
        RosettaDragon()
            : m_direction(West)
        {
            m_bitmap.Create(kBitmapSize, kBitmapSize);
        }

        bool Draw(int iterations, const std::wstring& outputPath)
        {
            Generate(iterations);
            return DrawToBitmap(outputPath);
        }

    private:
        void Generate(int iterations)
        {
            m_generator.clear();
            m_generator.push_back(1);

            std::string temp;
            for (int y = 0; y < iterations - 1; ++y)
            {
                temp = m_generator;
                temp.push_back(1);
                for (auto it = m_generator.rbegin(); it != m_generator.rend(); ++it)
                {
                    temp.push_back(static_cast<char>(!(*it)));
                }
                m_generator = temp;
            }
        }

        bool DrawToBitmap(const std::wstring& outputPath)
        {
            HDC dc = m_bitmap.GetDC();
            if (!dc)
            {
                return false;
            }

            const unsigned int colors[] = { 0x0000ff, 0x00ff00, 0xff0000, 0x00ffff };
            const int moveOffsets[] = { 0, 0, 1, -1, 1, -1, 1, 0 };
            int moveIndex = 0;

            m_bitmap.Clear();
            m_direction = West;

            for (int t = 0; t < 4; ++t)
            {
                int a = kBitmapSize / 2;
                int b = a;
                a += moveOffsets[moveIndex++];
                b += moveOffsets[moveIndex++];
                MoveToEx(dc, a, b, nullptr);

                m_bitmap.SetPenColor(colors[t]);
                for (char turn : m_generator)
                {
                    switch (m_direction)
                    {
                    case North:
                        if (turn)
                        {
                            a += kSegmentLength;
                            m_direction = East;
                        }
                        else
                        {
                            a -= kSegmentLength;
                            m_direction = West;
                        }
                        break;
                    case East:
                        if (turn)
                        {
                            b += kSegmentLength;
                            m_direction = South;
                        }
                        else
                        {
                            b -= kSegmentLength;
                            m_direction = North;
                        }
                        break;
                    case South:
                        if (turn)
                        {
                            a -= kSegmentLength;
                            m_direction = West;
                        }
                        else
                        {
                            a += kSegmentLength;
                            m_direction = East;
                        }
                        break;
                    case West:
                        if (turn)
                        {
                            b -= kSegmentLength;
                            m_direction = North;
                        }
                        else
                        {
                            b += kSegmentLength;
                            m_direction = South;
                        }
                        break;
                    default:
                        break;
                    }

                    LineTo(dc, a, b);
                }
            }

            return m_bitmap.SaveBitmap(outputPath);
        }

        int m_direction;
        RosettaBitmap m_bitmap;
        std::string m_generator;
    };
}

int RosettaDragonApp::Run()
{
    const std::filesystem::path outputDirectory = std::filesystem::current_path() / L"Output";
    std::error_code ec;
    std::filesystem::create_directories(outputDirectory, ec);

    const std::filesystem::path outputPath = outputDirectory / L"Example0001_RosettaDragon.bmp";

    RosettaDragon dragon;
    if (!dragon.Draw(kIterations, outputPath.wstring()))
    {
        std::wcerr << L"Failed to generate bitmap: " << outputPath << std::endl;
        return 1;
    }

    std::wcout << L"Wrote dragon curve bitmap to: " << outputPath << std::endl;
    return 0;
}
