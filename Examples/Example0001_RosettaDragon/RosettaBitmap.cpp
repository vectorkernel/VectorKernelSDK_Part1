#include "RosettaBitmap.h"

#include <cstring>

RosettaBitmap::RosettaBitmap()
    : m_bitmap(nullptr)
    , m_hdc(nullptr)
    , m_pen(nullptr)
    , m_brush(nullptr)
    , m_bits(nullptr)
    , m_width(0)
    , m_height(0)
    , m_penWidth(1)
    , m_penColor(0)
{
}

RosettaBitmap::~RosettaBitmap()
{
    if (m_pen)
    {
        DeleteObject(m_pen);
    }
    if (m_brush)
    {
        DeleteObject(m_brush);
    }
    if (m_hdc)
    {
        DeleteDC(m_hdc);
    }
    if (m_bitmap)
    {
        DeleteObject(m_bitmap);
    }
}

bool RosettaBitmap::Create(int width, int height)
{
    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biBitCount = sizeof(DWORD) * 8;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = -height;

    HDC dc = ::GetDC(GetConsoleWindow());
    m_bitmap = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &m_bits, nullptr, 0);
    if (!m_bitmap)
    {
        ReleaseDC(GetConsoleWindow(), dc);
        return false;
    }

    m_hdc = CreateCompatibleDC(dc);
    SelectObject(m_hdc, m_bitmap);
    ReleaseDC(GetConsoleWindow(), dc);

    m_width = width;
    m_height = height;
    return true;
}

void RosettaBitmap::Clear(BYTE clearValue)
{
    if (m_bits)
    {
        std::memset(m_bits, clearValue, static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * sizeof(DWORD));
    }
}

void RosettaBitmap::SetBrushColor(DWORD color)
{
    if (m_brush)
    {
        DeleteObject(m_brush);
    }
    m_brush = CreateSolidBrush(color);
    SelectObject(m_hdc, m_brush);
}

void RosettaBitmap::SetPenColor(DWORD color)
{
    m_penColor = color;
    CreatePen();
}

void RosettaBitmap::SetPenWidth(int width)
{
    m_penWidth = width;
    CreatePen();
}

bool RosettaBitmap::SaveBitmap(const std::wstring& path) const
{
    BITMAPFILEHEADER fileHeader{};
    BITMAPINFO infoHeader{};
    BITMAP bitmap{};
    DWORD writtenBytes = 0;

    GetObject(m_bitmap, sizeof(bitmap), &bitmap);
    DWORD* bitmapBits = new DWORD[static_cast<size_t>(bitmap.bmWidth) * static_cast<size_t>(bitmap.bmHeight)];

    std::memset(bitmapBits, 0, static_cast<size_t>(bitmap.bmWidth) * static_cast<size_t>(bitmap.bmHeight) * sizeof(DWORD));
    infoHeader.bmiHeader.biBitCount = sizeof(DWORD) * 8;
    infoHeader.bmiHeader.biCompression = BI_RGB;
    infoHeader.bmiHeader.biPlanes = 1;
    infoHeader.bmiHeader.biSize = sizeof(infoHeader.bmiHeader);
    infoHeader.bmiHeader.biHeight = bitmap.bmHeight;
    infoHeader.bmiHeader.biWidth = bitmap.bmWidth;
    infoHeader.bmiHeader.biSizeImage = bitmap.bmWidth * bitmap.bmHeight * sizeof(DWORD);

    fileHeader.bfType = 0x4D42;
    fileHeader.bfOffBits = sizeof(infoHeader.bmiHeader) + sizeof(BITMAPFILEHEADER);
    fileHeader.bfSize = fileHeader.bfOffBits + infoHeader.bmiHeader.biSizeImage;

    GetDIBits(m_hdc, m_bitmap, 0, m_height, reinterpret_cast<LPVOID>(bitmapBits), &infoHeader, DIB_RGB_COLORS);

    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        delete[] bitmapBits;
        return false;
    }

    WriteFile(file, &fileHeader, sizeof(BITMAPFILEHEADER), &writtenBytes, nullptr);
    WriteFile(file, &infoHeader.bmiHeader, sizeof(infoHeader.bmiHeader), &writtenBytes, nullptr);
    WriteFile(file, bitmapBits, bitmap.bmWidth * bitmap.bmHeight * 4, &writtenBytes, nullptr);
    CloseHandle(file);

    delete[] bitmapBits;
    return true;
}

void RosettaBitmap::CreatePen()
{
    if (m_pen)
    {
        DeleteObject(m_pen);
    }
    m_pen = ::CreatePen(PS_SOLID, m_penWidth, m_penColor);
    SelectObject(m_hdc, m_pen);
}
