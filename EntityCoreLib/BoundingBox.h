#pragma once

struct BoundingBox {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;

    BoundingBox() : minX(0), minY(0), minZ(0), maxX(0), maxY(0), maxZ(0) {}
    BoundingBox(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
        : minX(minX), minY(minY), minZ(minZ), maxX(maxX), maxY(maxY), maxZ(maxZ) {
    }
};

inline bool Intersects(const BoundingBox& a, const BoundingBox& b)
{
    return !(a.maxX < b.minX || a.minX > b.maxX ||
             a.maxY < b.minY || a.minY > b.maxY ||
             a.maxZ < b.minZ || a.minZ > b.maxZ);
}
