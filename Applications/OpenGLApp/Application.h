#pragma once

#include <glad/glad.h>

#include <vector>
#include <string_view>
#include <cstdint>
#include <memory>

#include "..\EntityCoreLib\VklExpected.h"
#include "..\EntityCoreLib\EntityBook.h"
#include "..\EntityCoreLib\LineEntity.h"
#include "..\EntityCoreLib\EntityBookFactory.h"

#include "WindowTypes.h"

class Application
{
public:
    Application();
    ~Application();

    void Initialize();
    void Update(float inDeltaTime);
    void Render(const ClientSize& clientSize);
    void Shutdown();

    static void LogSink(EntityCore::LogLevel level,
        const char* category,
        const char* event,
        std::string_view message);

private:
    struct Vertex
    {
        Vec3 pos;
        Vec4 color;
    };

    struct Mat4
    {
        float m[16]{};
    };

private:
    void BuildGrid(float modelHalfW, float modelHalfH);
    void UploadLinesToGpu();
    static Mat4 BuildOrtho(float left, float right, float bottom, float top, float nearZ, float farZ);

private:
    using EntityBookPtr = std::unique_ptr<EntityBook>;
    using EntityBookCreateResult = vkl::expected<EntityBookPtr, EntityBookError>;

    static EntityBookCreateResult CreateBook(std::size_t capacity);

    EntityBookPtr m_book;
    uint32_t m_nextId = 1;

    GLuint m_program = 0;
    GLint  m_uMvp = -1;
    GLuint m_vbo = 0;

    std::vector<Vertex> m_vertices;

    ClientSize m_lastClientSize{};

    float m_viewHalfHeight = 10.0f;
    float m_viewHalfWidth = 10.0f;
};
