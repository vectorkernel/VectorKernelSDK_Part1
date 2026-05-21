#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <glad/glad.h>

#include <vector>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <string>
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
#include "..\RenderCoreLib\LayerTable.h"
#include "..\RenderCoreLib\hersheyfont.h"
#include "..\AppCoreLib\SelectionController.h"
#include "..\AppCoreLib\SelectionStates.h"
#include "..\AppCoreLib\PropertiesWindow.h"
#include "..\AppCoreLib\LayersDialogSimple.h"
#include "..\AppCoreLib\DialogContracts.h"
#include "..\AppCoreLib\HersheyCommandWindow.h"
#include "..\AppCoreLib\HudJigRect.h"

class Application : public IPropertiesHost, public AppCore::IHersheyCommandHost
{
public:
    Application();

    HINSTANCE m_instance = nullptr;
    HWND m_mainHwnd = nullptr;
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
    void OnChar(unsigned int ch);
    void ResetView();
    void OnLayerDialogStateChanged();

    static void LogSink(EntityCore::LogLevel level,
                        const char* category,
                        const char* event,
                        std::string_view message);

    void OnHersheyCommandSubmitted(const std::string& commandText) override;
    void RequestHersheyCommandRedraw() override;

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
    void RebuildLineEntityIndex();

    SelectionHit FindBestEntityHitAtClient(int clientX, int clientY) const;
    void AppendGlowOverlayForLine(const LineEntity& line, const glm::vec4& color, float offsetPx);
    void AppendGripOverlayForLine(const LineEntity& line, const glm::vec4& color, float halfSizePx);
    void AppendGlowOverlayForEntity(std::size_t entityId, const glm::vec4& color, float offsetPx);
    void AppendGripOverlayForEntity(std::size_t entityId, const glm::vec4& color, float halfSizePx);
    void AppendBoxSelectionOverlay();
    void ApplyActiveBoxSelection();
    void AppendHudJigOverlay();
    void CancelActiveHudJig(const char* reason = nullptr);
    void BeginZoomWindowHudJig();
    void ZoomExtents();
    void ZoomToWorldWindow(const glm::vec2& worldMin, const glm::vec2& worldMax, float paddingFactor = 1.05f);
    bool IsZoomWindowHudJigActive() const;

    static float DistancePointToSegmentPx(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b, float& outT);
    static bool SegmentIntersectsAabb2D(const glm::vec2& a, const glm::vec2& b, const glm::vec2& mn, const glm::vec2& mx);
    static float DistanceClientPx(int x0, int y0, int x1, int y1);

    bool IsCtrlDown() const;
    void SyncPropertiesWindow();
    std::string BuildEntityDisplayName(const Entity& entity) const;
    std::string BuildColorName(const glm::vec4& color) const;
    const LineEntity* FindLineById(std::size_t entityId) const;
    LineEntity* FindLineById(std::size_t entityId);
    std::string BuildCommandPrompt() const;
    std::string BuildCommandStatus() const;
    void AddCommandHistory(const std::string& line);

    std::vector<SelectedEntityInfo> BuildSelectedEntities() const override;
    bool TryGetSingleSelectedLine(SingleLineSelectionInfo& outInfo) const override;
    bool UpdateSingleSelectedLine(const SingleLineSelectionInfo& info) override;
    std::vector<std::pair<std::string, std::uint32_t>> BuildLayerChoices() const override;
    bool AssignSelectionLayer(std::uint32_t layerId, EntityKind filterKind) override;

private:
    EntityBookPtr m_book;
    std::size_t m_nextId = 1;

    GLuint m_program = 0;
    GLint  m_uMvp = -1;
    GLuint m_sceneVbo = 0;
    GLuint m_overlayLineVbo = 0;
    GLuint m_overlayTriVbo = 0;

    std::vector<Vertex> m_sceneVertices;
    std::vector<Vertex> m_overlayLineVertices;
    std::vector<Vertex> m_overlayTriVertices;

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
    std::unordered_map<std::size_t, const LineEntity*> m_lineEntityIndex;

    RenderCore::DragonBuildResult m_dragon;
    bool m_needsFitToClient = true;

    PropertiesWindow m_propertiesWindow;
    LayersDialogSimple m_layersDialog;
    LayerTable m_layerTable;

    AppCore::HersheyCommandWindow m_commandWindow;
    hershey_font* m_commandFont = nullptr;
    AppCore::HudJigRect m_hudJigRect;
    glm::vec4 m_zoomWindowHudJigColor{ 1.0f, 0.5f, 0.0f, 1.0f };
};
