#pragma once

#include <glad/glad.h>

#include <vector>
#include <cstdint>
#include <memory>
#include <glm/glm.hpp>

#include "WindowTypes.h"

#include "..\EntityCoreLib\VklExpected.h"
#include "..\EntityCoreLib\EntityBook.h"
#include "..\EntityCoreLib\LineEntity.h"
#include "..\EntityCoreLib\EntityBookFactory.h"
#include "..\RenderCoreLib\PanZoomController.h"
#include "..\RenderCoreLib\GridUtil.h"
#include "..\RenderCoreLib\Crosshairs.h"

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
    void OnMouseLeave();
    void OnMouseWheel(int x, int y, int wheelDelta);
    void OnKeyDown(unsigned int vk);
    void ResetView();

private:
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec4 color;
    };

private:
    using EntityBookPtr = std::unique_ptr<EntityBook>;
    using EntityBookCreateResult = vkl::expected<EntityBookPtr, EntityBookError>;

    static EntityBookCreateResult CreateBook(std::size_t capacity);

    bool RebuildGridIfNeeded();
    void RebuildOverlayLines();
    void UploadSceneToGpu();
    void UploadOverlayToGpu();

    static float SnapScalarToGrid(float value, float spacing);
    glm::vec2 SnapWorldToGrid(const glm::vec2& world) const;
    void RefreshSnappedMousePosition();

private:
    EntityBookPtr m_book;
    std::size_t m_nextId = 1;

    GLuint m_program = 0;
    GLint  m_uMvp = -1;
    GLuint m_sceneVbo = 0;
    GLuint m_overlayVbo = 0;

    std::vector<Vertex> m_sceneVertices;
    std::vector<Vertex> m_overlayVertices;

    ClientSize m_clientSize{};

    PanZoomController m_panZoom;
    RenderCore::GridCache m_gridCache;
    float m_gridSpacingWorld = 10.0f;
    int m_gridMajorEvery = 5;

    RenderCore::Crosshairs m_crosshairs;
    bool m_mouseInsideClient = false;
    bool m_gridSnapEnabled = true;
    bool m_rightButtonPanning = false;
    int m_rawMouseX = 0;
    int m_rawMouseY = 0;
};
