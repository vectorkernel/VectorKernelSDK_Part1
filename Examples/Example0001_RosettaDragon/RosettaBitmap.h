#pragma once

#include <string>
#include <windows.h>

class RosettaBitmap
{
public:
    RosettaBitmap();
    ~RosettaBitmap();

    RosettaBitmap(const RosettaBitmap&) = delete;
    RosettaBitmap& operator=(const RosettaBitmap&) = delete;

    bool Create(int width, int height);
    void Clear(BYTE clearValue = 0);
    void SetBrushColor(DWORD color);
    void SetPenColor(DWORD color);
    void SetPenWidth(int width);
    bool SaveBitmap(const std::wstring& path) const;

    HDC GetDC() const { return m_hdc; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

private:
    void CreatePen();

    HBITMAP m_bitmap;
    HDC m_hdc;
    HPEN m_pen;
    HBRUSH m_brush;
    void* m_bits;
    int m_width;
    int m_height;
    int m_penWidth;
    DWORD m_penColor;
};
