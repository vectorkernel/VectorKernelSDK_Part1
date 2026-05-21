#pragma once

#include <glad/glad.h>

#include <vector>
#include <cstdint>
#include <memory>
#include <string_view>

#include <glm/glm.hpp>

#include "WindowTypes.h"

#include "..\EntityCoreLib\VklExpected.h"
#include "..\EntityCoreLib\EntityBook.h"
#include "..\EntityCoreLib\LineEntity.h"
#include "..\EntityCoreLib\EntityBookFactory.h"
#include "..\EntityCoreLib\EntityCoreLog.h"
#include "..\RenderCoreLib\PanZoomController.h"
#include "..\RenderCoreLib\GridUtil.h"
#include "..\RenderCoreLib\Crosshairs.h"
#include "..\RenderCoreLib\DragonGeneration.h"
#include "..\AppCoreLib\SelectionController.h"

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
    void OnLMouseButtonDown(int x, int y);
    void OnLMouseButtonUp(int x, int y);
    void OnRMouseButtonDown(int x, int y);
    void OnRMouseButtonUp(int x, int y);
    void OnMouseMove(int x, int y, bool leftButtonDown, bool rightButtonDown, bool shiftDown);
    void OnMouseLeave();
    void OnMouseWheel(int x, int y, int wheelDelta);
    void OnKeyDown(unsigned int vk);
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

private:
    using EntityBookPtr = std::unique_ptr<EntityBook>;
    using EntityBookCreateResult = vkl::expected<EntityBookPtr, EntityBookError>;

    static EntityBookCreateResult CreateBook(std::size_t capacity);

    void BuildDragonScene();
    bool RebuildGridIfNeeded();
    void RebuildOverlayLines();
    void UploadSceneToGpu();
    void UploadOverlayToGpu();
    void FitDragonToClient();

    static float SnapScalarToGrid(float value, float spacing);
    glm::vec2 SnapWorldToGrid(const glm::vec2& world) const;
    void RefreshSnappedMousePosition();
    void UpdateHoveredEntity();

    SelectionHit FindBestEntityHitAtClient(int clientX, int clientY) const;
    void AppendGlowOverlayForEntity(std::size_t entityId, const glm::vec4& color, float offsetPx);
    void AppendGripOverlayForEntity(std::size_t entityId, const glm::vec4& color, float halfSizePx);
    void AppendBoxSelectionOverlay();
    void ApplyActiveBoxSelection();

    static float DistancePointToSegmentPx(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b, float& outT);
    static bool SegmentIntersectsAabb2D(const glm::vec2& a, const glm::vec2& b, const glm::vec2& mn, const glm::vec2& mx);
    static float DistanceClientPx(int x0, int y0, int x1, int y1);

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
    bool m_gridSnapEnabled = false;
    bool m_leftButtonDown = false;
    bool m_rightButtonDown = false;
    bool m_pendingLeftCrossing = false;
    bool m_pendingRightBox = false;
    bool m_panWithRightDrag = false;
    int m_rawMouseX = 0;
    int m_rawMouseY = 0;
    int m_dragStartClientX = 0;
    int m_dragStartClientY = 0;
    float m_dragStartThresholdPx = 4.0f;

    SelectionController m_selection;
    glm::vec4 m_hoverColor{ 1.0f, 1.0f, 1.0f, 1.0f };
    glm::vec4 m_selectedColor{ 0.0f, 1.0f, 1.0f, 1.0f };
    glm::vec4 m_gripColor{ 1.0f, 1.0f, 0.0f, 1.0f };
    glm::vec4 m_crossingBoxColor{ 0.0f, 1.0f, 0.0f, 1.0f };
    glm::vec4 m_windowBoxColor{ 0.2f, 0.7f, 1.0f, 1.0f };
    float m_hoverPickRadiusPx = 8.0f;

    RenderCore::DragonBuildResult m_dragon;
    bool m_needsFitToClient = true;
};
