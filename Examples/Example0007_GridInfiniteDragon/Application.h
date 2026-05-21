#pragma once

#include <glad/glad.h>

#include <memory>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

#include "WindowTypes.h"

#include "..\..\EntityCoreLib\EntityBook.h"
#include "..\..\EntityCoreLib\EntityBookFactory.h"
#include "..\..\EntityCoreLib\LineEntity.h"
#include "..\..\EntityCoreLib\VklExpected.h"
#include "..\..\RenderCoreLib\DragonGeneration.h"
#include "..\..\RenderCoreLib\PanZoomController.h"

class Application
{
public:
    Application();
    ~Application();

    void Initialize();
    void Update(float inDeltaTime);
    void Render(const ClientSize& clientSize);
    void Shutdown();

    void OnResize(const ClientSize& clientSize);
    void OnRMouseButtonDown(int x, int y);
    void OnRMouseButtonUp();
    void OnMouseMove(int x, int y, bool rightButtonDown);
    void OnMouseWheel(int x, int y, int wheelDelta);
    void ResetView();

    static void LogSink(EntityCore::LogLevel level,
                        const char* category,
                        const char* event,
                        std::string_view message);

private:
    struct Vertex
    {
        glm::vec3 pos{};
        glm::vec4 color{ 1.0f };
    };

    using EntityBookPtr = std::unique_ptr<EntityBook>;
    using EntityBookCreateResult = vkl::expected<EntityBookPtr, EntityBookError>;

    static EntityBookCreateResult CreateBook(std::size_t capacity);

    void BuildDragonScene();
    void UploadLinesToGpu();
    void FitDragonToClient();

private:
    EntityBookPtr m_book;
    std::size_t m_nextId = 1;

    GLuint m_program = 0;
    GLint  m_uMvp = -1;
    GLuint m_vbo = 0;

    std::vector<Vertex> m_vertices;
    ClientSize m_clientSize{};
    PanZoomController m_panZoom;
    RenderCore::DragonBuildResult m_dragon;
    bool m_needsFitToClient = true;
};
