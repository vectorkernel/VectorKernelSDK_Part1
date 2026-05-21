#include "Application.h"

#include <sstream>

#include <iostream>
#include <cstddef>
#include <utility>
#include <algorithm>
#include <cmath>
#include <array>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "..\RenderCoreLib\GLShaderUtil.h"

Application::Application() = default;

Application::~Application()
{
    Shutdown();
}

Application::EntityBookCreateResult Application::CreateBook(std::size_t capacity)
{
    return CreateEntityBook(capacity);
}

void Application::LogSink(EntityCore::LogLevel level,
                          const char* category,
                          const char* event,
                          std::string_view message)
{
    if (level == EntityCore::LogLevel::Trace &&
        category != nullptr &&
        event != nullptr &&
        std::string_view(category) == "EntityBook" &&
        std::string_view(event) == "AddEntity" &&
        (message.find("tag=Scene") != std::string_view::npos ||
         message.find("tag=Grid") != std::string_view::npos))
    {
        return;
    }

    const char* lvl = "TRACE";
    switch (level)
    {
    case EntityCore::LogLevel::Trace: lvl = "TRACE"; break;
    case EntityCore::LogLevel::Info:  lvl = "INFO "; break;
    case EntityCore::LogLevel::Warn:  lvl = "WARN "; break;
    case EntityCore::LogLevel::Error: lvl = "ERROR"; break;
    default: break;
    }

    std::cout << '[' << lvl << "] " << (category ? category : "(null)")
              << "::" << (event ? event : "(null)");
    if (!message.empty())
        std::cout << " | " << message;
    std::cout << '\n';
}

void Application::Initialize()
{
    EntityCore::SetLogSink(&Application::LogSink);
    std::cout << "Application::Initialize\n";

    EntityBookCreateResult created = CreateBook(200000);
    if (!created.has_value())
    {
        std::cout << "FATAL: EntityBook creation failed";
        if (!created.error().message.empty())
            std::cout << ": " << created.error().message;
        std::cout << "\n";
        return;
    }

    m_book = std::move(created.value());

    const char* vsSrc = R"GLSL(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec4 aColor;

        uniform mat4 uMvp;
        out vec4 vColor;

        void main()
        {
            vColor = aColor;
            gl_Position = uMvp * vec4(aPos, 1.0);
        }
    )GLSL";

    const char* fsSrc = R"GLSL(
        #version 330 core
        in vec4 vColor;
        out vec4 FragColor;

        void main()
        {
            FragColor = vColor;
        }
    )GLSL";

    m_program = GLShaderUtil::create(vsSrc, fsSrc);
    if (m_program == 0)
    {
        std::cout << "FATAL: GLShaderUtil::create failed\n";
        return;
    }

    m_uMvp = glGetUniformLocation(m_program, "uMvp");
    glGenBuffers(1, &m_sceneVbo);
    glGenBuffers(1, &m_overlayVbo);

    m_panZoom.mode = PanZoomController::OrthoMode::Center;
    m_panZoom.minZoom = 0.02f;
    m_panZoom.maxZoom = 100.0f;
    m_panZoom.wheelZoomFactor = 1.12f;
    m_panZoom.dragSpeed = 1.0f;

    m_crosshairs.SetViewport(1, 1);
    m_crosshairs.SetPickBoxSizePx(12);
    m_crosshairs.SetBackground(RenderCore::CrosshairsBackground::Dark);
    m_crosshairs.SetMode(RenderCore::CrosshairsMode::LinesAndPickBox);
    m_selection.SetMode(SelectionMode::Grips);

    m_layerTable.ResetToDefault();
    m_layerTable.AddLayer("Dragon Red");
    m_layerTable.AddLayer("Dragon Green");
    m_layerTable.AddLayer("Dragon Blue");
    m_layerTable.AddLayer("Dragon Yellow");

    BuildDragonScene();
    ResetView();

    m_propertiesWindow.Create(m_instance, m_mainHwnd, this, CW_USEDEFAULT, CW_USEDEFAULT, 460, 480);

    std::cout << "Snap grid demo controls:\n"
              << "  Hover over dragon segments to highlight them (HUD overlay).\n"
              << "  X  : cycle crosshair mode\n"
              << "  B  : cycle background preset\n"
              << "  R  : reset view\n"
              << "  RMB drag left  : crossing selection\n"
              << "  RMB drag right : window selection\n"
              << "  Shift+RMB drag : pan\n"
              << "  Wheel    : zoom\n";
}

void Application::BuildDragonScene()
{
    if (!m_book)
        return;

    m_book->Clear();
    m_nextId = 1;
    m_gridCache = {};

    constexpr int kOrder = 15;
    constexpr float kStep = 1.0f;

    std::vector<int> turns;
    turns.reserve((1u << kOrder) - 1u);
    turns.push_back(1);
    for (int iteration = 0; iteration < kOrder - 1; ++iteration)
    {
        std::vector<int> next = turns;
        next.reserve((turns.size() * 2u) + 1u);
        next.push_back(1);
        for (auto it = turns.rbegin(); it != turns.rend(); ++it)
            next.push_back(*it ? 0 : 1);
        turns.swap(next);
    }

    constexpr std::array<glm::ivec2, 4> kOffsets = {{
        { 0, 0 },
        { 1, -1 },
        { 1, -1 },
        { 1, 0 },
    }};

    constexpr std::array<glm::vec4, 4> kColors = {{
        glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
        glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),
    }};

    enum class LocalDir
    {
        North = 1,
        East = 2,
        South = 4,
        West = 8
    };

    auto advanceOne = [](glm::vec2& pt, LocalDir dir, float step)
    {
        switch (dir)
        {
        case LocalDir::North: pt.y -= step; break;
        case LocalDir::East:  pt.x += step; break;
        case LocalDir::South: pt.y += step; break;
        case LocalDir::West:  pt.x -= step; break;
        }
    };

    std::vector<RenderCore::DragonSegment> segments;
    segments.reserve(static_cast<std::size_t>(turns.size()) * kColors.size());

    glm::vec2 minPt(0.0f);
    glm::vec2 maxPt(0.0f);
    bool hasBounds = false;

    auto includePoint = [&](const glm::vec2& pt)
    {
        if (!hasBounds)
        {
            minPt = pt;
            maxPt = pt;
            hasBounds = true;
        }
        else
        {
            minPt = glm::min(minPt, pt);
            maxPt = glm::max(maxPt, pt);
        }
    };

    LocalDir direction = LocalDir::West;

    for (std::size_t passIndex = 0; passIndex < kColors.size(); ++passIndex)
    {
        glm::vec2 current(static_cast<float>(kOffsets[passIndex].x),
                          static_cast<float>(kOffsets[passIndex].y));
        includePoint(current);

        for (const int turn : turns)
        {
            switch (direction)
            {
            case LocalDir::North:
                direction = turn ? LocalDir::East : LocalDir::West;
                break;
            case LocalDir::East:
                direction = turn ? LocalDir::South : LocalDir::North;
                break;
            case LocalDir::South:
                direction = turn ? LocalDir::West : LocalDir::East;
                break;
            case LocalDir::West:
                direction = turn ? LocalDir::North : LocalDir::South;
                break;
            }

            glm::vec2 next = current;
            advanceOne(next, direction, kStep);
            includePoint(next);

            RenderCore::DragonSegment segment;
            segment.a = current;
            segment.b = next;
            segment.color = kColors[passIndex];
            segments.push_back(segment);

            current = next;
        }
    }

    if (!hasBounds || segments.empty())
    {
        std::cout << "FATAL: dragon generation failed\n";
        return;
    }

    m_dragon = {};
    m_dragon.segments = std::move(segments);
    m_dragon.minPt = minPt;
    m_dragon.maxPt = maxPt;
    m_dragon.center = 0.5f * (minPt + maxPt);
    m_dragon.pointCount = static_cast<std::size_t>(turns.size() + 1u) * kColors.size();
    m_dragon.workerCountUsed = 1;
    m_dragon.hasGeometry = true;

    for (RenderCore::DragonSegment& segment : m_dragon.segments)
    {
        segment.a -= m_dragon.center;
        segment.b -= m_dragon.center;
    }
    m_dragon.minPt -= m_dragon.center;
    m_dragon.maxPt -= m_dragon.center;
    m_dragon.center = glm::vec2(0.0f, 0.0f);

    m_book->entities.reserve(m_dragon.segments.size() + 2048u);

    for (const RenderCore::DragonSegment& segment : m_dragon.segments)
    {
        auto line = std::make_unique<LineEntity>();
        line->ID = m_nextId++;
        line->tag = EntityTag::Scene;
        line->drawOrder = 100;
        line->screenSpace = false;
        line->pickable = true;
        line->visible = true;
        line->selected = false;
        line->p0 = glm::vec3(segment.a, 0.0f);
        line->p1 = glm::vec3(segment.b, 0.0f);
        line->color = segment.color;
        line->width = 1.0f;
        const float r = segment.color.r;
        const float g = segment.color.g;
        const float b = segment.color.b;
        if (r > 0.9f && g < 0.2f && b < 0.2f)
            line->layerId = 2;
        else if (g > 0.9f && r < 0.2f && b < 0.2f)
            line->layerId = 3;
        else if (b > 0.9f && r < 0.2f && g < 0.2f)
            line->layerId = 4;
        else
            line->layerId = 5;
        m_book->entities.push_back(std::move(line));
    }

    m_book->SortByDrawOrder();
    RebuildLineEntityIndex();
    UploadSceneToGpu();
    m_needsFitToClient = true;

    std::cout << "Dragon built: " << m_dragon.segments.size()
              << " segments across " << kColors.size()
              << " Rosetta passes using 1 worker(s).\n";
}


void Application::RebuildLineEntityIndex()
{
    m_lineEntityIndex.clear();

    if (!m_book)
        return;

    m_lineEntityIndex.reserve(m_book->entities.size());

    for (const auto& ep : m_book->entities)
    {
        if (!ep || ep->type != EntityType::Line)
            continue;

        const auto* line = ep->As<LineEntity>();
        if (line)
            m_lineEntityIndex[line->ID] = line;
    }
}

void Application::Update(float /*inDeltaTime*/)
{
    RebuildGridIfNeeded();
    RebuildOverlayLines();
    UploadOverlayToGpu();
}

void Application::Render(const ClientSize& clientSize)
{
    if (!m_book || !clientSize.IsValid())
        return;

    if (clientSize != m_clientSize)
        OnResize(clientSize);

    const RenderCore::CrosshairsColors colors = m_crosshairs.GetColors();

    glViewport(0, 0, m_clientSize.width, m_clientSize.height);
    glClearColor(colors.clearColor.r, colors.clearColor.g, colors.clearColor.b, colors.clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_program);

    const glm::mat4 sceneMvp = m_panZoom.GetMVP();
    glUniformMatrix4fv(m_uMvp, 1, GL_FALSE, glm::value_ptr(sceneMvp));

    glBindBuffer(GL_ARRAY_BUFFER, m_sceneVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_sceneVertices.size()));

    const glm::mat4 overlayMvp = glm::ortho(
        0.0f,
        static_cast<float>(m_clientSize.width),
        static_cast<float>(m_clientSize.height),
        0.0f,
        -1.0f,
        1.0f);

    glUniformMatrix4fv(m_uMvp, 1, GL_FALSE, glm::value_ptr(overlayMvp));

    glBindBuffer(GL_ARRAY_BUFFER, m_overlayVbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, pos)));
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_overlayVertices.size()));
}

void Application::Shutdown()
{
    if (m_program == 0 && m_sceneVbo == 0 && m_overlayVbo == 0 && !m_book)
        return;

    std::cout << "Application::Shutdown\n";

    EntityCore::SetLogSink(nullptr);

    if (m_sceneVbo)
    {
        glDeleteBuffers(1, &m_sceneVbo);
        m_sceneVbo = 0;
    }

    if (m_overlayVbo)
    {
        glDeleteBuffers(1, &m_overlayVbo);
        m_overlayVbo = 0;
    }

    if (m_program)
    {
        glDeleteProgram(m_program);
        m_program = 0;
    }

    m_sceneVertices.clear();
    m_overlayVertices.clear();

    m_propertiesWindow.Destroy();

    if (m_book)
    {
        m_book->Clear();
        m_book.reset();
    }
}

void Application::OnResize(const ClientSize& clientSize)
{
    m_clientSize = clientSize;
    m_panZoom.SetViewport(clientSize.width, clientSize.height);
    m_crosshairs.SetViewport(clientSize.width, clientSize.height);
    FitDragonToClient();
    RebuildGridIfNeeded();
    RefreshSnappedMousePosition();
    UpdateHoveredEntity();
    SyncPropertiesWindow();
    RebuildOverlayLines();
    UploadOverlayToGpu();
}

void Application::OnLMouseButtonDown(int x, int y)
{
    m_leftButtonDown = true;
    m_rawMouseX = x;
    m_rawMouseY = y;
    m_dragStartClientX = x;
    m_dragStartClientY = y;

    RefreshSnappedMousePosition();

    const SelectionHit hit = FindBestEntityHitAtClient(x, y);
    if (hit.IsValid())
    {
        m_selection.Toggle(hit.id);
        m_pendingLeftCrossing = false;
    }
    else
    {
        m_pendingLeftCrossing = true;
    }

    UpdateHoveredEntity();
    SyncPropertiesWindow();
    RebuildOverlayLines();
    UploadOverlayToGpu();
}

void Application::OnLMouseButtonUp(int x, int y)
{
    m_leftButtonDown = false;
    m_rawMouseX = x;
    m_rawMouseY = y;

    if (m_selection.IsCrossingBoxSelection())
    {
        m_selection.UpdateBox(m_panZoom.ScreenToWorld(x, y));
        ApplyActiveBoxSelection();
        m_selection.CancelBox();
    }

    m_pendingLeftCrossing = false;
    RefreshSnappedMousePosition();
    UpdateHoveredEntity();
    SyncPropertiesWindow();
    RebuildOverlayLines();
    UploadOverlayToGpu();
}

void Application::OnRMouseButtonDown(int x, int y)
{
    m_rightButtonDown = true;
    m_rawMouseX = x;
    m_rawMouseY = y;
    m_dragStartClientX = x;
    m_dragStartClientY = y;
    m_panWithRightDrag = ((GetKeyState(VK_SHIFT) & 0x8000) != 0);
    m_pendingRightBox = !m_panWithRightDrag;

    if (m_panWithRightDrag)
    {
        m_pendingRightBox = false;
        m_panZoom.OnMouseDown(x, y);
    }

    RefreshSnappedMousePosition();
    UpdateHoveredEntity();
    SyncPropertiesWindow();
    RebuildOverlayLines();
    UploadOverlayToGpu();
}

void Application::OnRMouseButtonUp(int x, int y)
{
    m_rightButtonDown = false;
    m_rawMouseX = x;
    m_rawMouseY = y;

    if (m_panWithRightDrag)
    {
        m_panZoom.OnMouseUp();
    }
    else if (m_selection.IsBoxSelectionActive())
    {
        m_selection.UpdateBox(m_panZoom.ScreenToWorld(x, y));
        ApplyActiveBoxSelection();
        m_selection.CancelBox();
    }

    m_pendingRightBox = false;
    m_panWithRightDrag = false;
    RefreshSnappedMousePosition();
    UpdateHoveredEntity();
    SyncPropertiesWindow();
    RebuildOverlayLines();
    UploadOverlayToGpu();
}

void Application::OnMouseMove(int x, int y, bool leftButtonDown, bool rightButtonDown, bool shiftDown)
{
    m_mouseInsideClient = true;
    m_rawMouseX = x;
    m_rawMouseY = y;
    m_leftButtonDown = leftButtonDown;
    m_rightButtonDown = rightButtonDown;

    const float dragDistance = DistanceClientPx(m_dragStartClientX, m_dragStartClientY, x, y);
    const glm::vec2 world = m_panZoom.ScreenToWorld(x, y);

    if (m_panWithRightDrag && rightButtonDown)
    {
        m_needsFitToClient = false;
        m_panZoom.OnMouseMove(x, y);
        RebuildGridIfNeeded();
        RefreshSnappedMousePosition();
        UpdateHoveredEntity();
    }
    else
    {
        if (m_pendingLeftCrossing && leftButtonDown && dragDistance >= m_dragStartThresholdPx && !m_selection.IsBoxSelectionActive())
            m_selection.BeginCrossingBox(m_panZoom.ScreenToWorld(m_dragStartClientX, m_dragStartClientY));

        if (m_pendingRightBox && rightButtonDown && dragDistance >= m_dragStartThresholdPx && !m_selection.IsBoxSelectionActive())
        {
            const bool crossing = x < m_dragStartClientX;
            if (crossing)
                m_selection.BeginCrossingBox(m_panZoom.ScreenToWorld(m_dragStartClientX, m_dragStartClientY));
            else
                m_selection.BeginWindowBox(m_panZoom.ScreenToWorld(m_dragStartClientX, m_dragStartClientY));
        }

        if (m_selection.IsBoxSelectionActive())
        {
            if (rightButtonDown && !m_panWithRightDrag)
            {
                const bool desiredCrossing = x < m_dragStartClientX;
                const bool currentCrossing = m_selection.GetBoxSelectionState().crossing;
                if (desiredCrossing != currentCrossing)
                {
                    const glm::vec2 startWorld = m_panZoom.ScreenToWorld(m_dragStartClientX, m_dragStartClientY);
                    m_selection.CancelBox();
                    if (desiredCrossing)
                        m_selection.BeginCrossingBox(startWorld);
                    else
                        m_selection.BeginWindowBox(startWorld);
                }
            }

            m_selection.UpdateBox(world);
            UpdateHoveredEntity();
        }
        else
        {
            RefreshSnappedMousePosition();
            UpdateHoveredEntity();
        }

        if (rightButtonDown && shiftDown && !m_panWithRightDrag && !m_selection.IsBoxSelectionActive() && !m_pendingRightBox)
        {
            m_panWithRightDrag = true;
            m_panZoom.OnMouseDown(m_dragStartClientX, m_dragStartClientY);
        }
    }

    RebuildOverlayLines();
    UploadOverlayToGpu();
}

void Application::OnMouseLeave()
{
    m_mouseInsideClient = false;
    m_selection.ClearHover();
    RebuildOverlayLines();
    UploadOverlayToGpu();
}

void Application::OnMouseWheel(int x, int y, int wheelDelta)
{
    m_needsFitToClient = false;
    m_panZoom.OnMouseWheel(x, y, wheelDelta);
    RebuildGridIfNeeded();
    RefreshSnappedMousePosition();
    UpdateHoveredEntity();
    SyncPropertiesWindow();
    RebuildOverlayLines();
    UploadOverlayToGpu();
}

void Application::OnKeyDown(unsigned int vk)
{
    switch (vk)
    {

    case 'X':
        m_crosshairs.ToggleMode();
        RebuildOverlayLines();
        UploadOverlayToGpu();
        std::cout << "Crosshairs mode: " << RenderCore::Crosshairs::ModeName(m_crosshairs.GetMode()) << "\n";
        break;

    case 'B':
        m_crosshairs.NextBackground();
        RebuildOverlayLines();
        UploadOverlayToGpu();
        std::cout << "Background: " << RenderCore::Crosshairs::BackgroundName(m_crosshairs.GetBackground()) << "\n";
        break;

    case '1':
        if (IsCtrlDown())
        {
            m_propertiesWindow.Toggle();
            SyncPropertiesWindow();
        }
        break;

    case 'R':
        ResetView();
        break;

    case 'C':
        m_selection.ClearSelection();
        SyncPropertiesWindow();
        RebuildOverlayLines();
        UploadOverlayToGpu();
        std::cout << "Selection cleared\n";
        break;

    case VK_ESCAPE:
        m_selection.ClearSelection();
        m_selection.ClearHover();
        m_selection.CancelBox();
        SyncPropertiesWindow();
        m_pendingLeftCrossing = false;
        m_pendingRightBox = false;
        if (m_panWithRightDrag)
            m_panZoom.OnMouseUp();
        m_panWithRightDrag = false;
        RebuildOverlayLines();
        UploadOverlayToGpu();
        std::cout << "Selection cleared (ESC)\n";
        break;

    default:
        break;
    }
}

void Application::ResetView()
{
    m_panZoom.pan = { 0.0f, 0.0f };
    m_panZoom.zoom = 1.0f;
    m_needsFitToClient = true;
    m_gridCache = {};
    FitDragonToClient();
    RebuildGridIfNeeded();
    RefreshSnappedMousePosition();
    UpdateHoveredEntity();
    SyncPropertiesWindow();
    RebuildOverlayLines();
    UploadOverlayToGpu();
}

bool Application::RebuildGridIfNeeded()
{
    if (!m_book || !m_clientSize.IsValid())
        return false;

    const bool rebuilt = RenderCore::UpdateBackgroundGrid(
        *m_book,
        m_panZoom,
        m_clientSize.width,
        m_clientSize.height,
        m_nextId,
        m_gridCache,
        m_gridSpacingWorld,
        m_gridMajorEvery);

    if (rebuilt)
        UploadSceneToGpu();

    return rebuilt;
}


void Application::RebuildOverlayLines()
{
    m_overlayVertices.clear();

    std::vector<LineEntity> crosshairLines;
    if (m_mouseInsideClient)
        m_crosshairs.BuildLines(crosshairLines);

    m_overlayVertices.reserve((crosshairLines.size() * 2) + (m_selection.GetSelectionCount() * 12) + 256);

    for (const LineEntity& line : crosshairLines)
    {
        if (!line.visible)
            continue;

        m_overlayVertices.push_back({ line.p0, line.color });
        m_overlayVertices.push_back({ line.p1, line.color });
    }

    const std::size_t hoverId = m_selection.GetHoverEntityId();
    if (hoverId != 0 && !m_selection.Contains(hoverId))
        AppendGlowOverlayForEntity(hoverId, m_hoverColor, 1.5f);

    for (std::size_t id : m_selection.GetSelectedEntities())
    {
        const LineEntity* line = FindLineById(id);
        if (!line || !line->visible || !m_layerTable.IsLayerVisible(line->layerId))
            continue;

        AppendGlowOverlayForLine(*line, m_selectedColor, 2.0f);
        if (m_selection.IsGripsSelectionMode())
            AppendGripOverlayForLine(*line, m_gripColor, 3.0f);
    }

    AppendBoxSelectionOverlay();
}

void Application::UploadSceneToGpu()
{
    if (!m_book)
        return;

    m_sceneVertices.clear();
    m_sceneVertices.reserve(m_book->entities.size() * 2);

    for (const auto& ep : m_book->entities)
    {
        if (!ep || ep->type != EntityType::Line)
            continue;

        const auto* line = ep->As<LineEntity>();
        if (!line || !line->visible || !m_layerTable.IsLayerVisible(line->layerId))
            continue;

        m_sceneVertices.push_back({ line->p0, line->color });
        m_sceneVertices.push_back({ line->p1, line->color });
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_sceneVbo);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_sceneVertices.size() * sizeof(Vertex)),
        m_sceneVertices.empty() ? nullptr : m_sceneVertices.data(),
        GL_STATIC_DRAW);
}

void Application::UploadOverlayToGpu()
{
    glBindBuffer(GL_ARRAY_BUFFER, m_overlayVbo);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_overlayVertices.size() * sizeof(Vertex)),
        m_overlayVertices.empty() ? nullptr : m_overlayVertices.data(),
        GL_DYNAMIC_DRAW);
}

void Application::FitDragonToClient()
{
    if (!m_needsFitToClient || !m_clientSize.IsValid() || !m_dragon.hasGeometry)
        return;

    const glm::vec2 extents = m_dragon.maxPt - m_dragon.minPt;
    const float width = std::max(extents.x, 1.0f);
    const float height = std::max(extents.y, 1.0f);
    const float paddedWidth = width * 1.10f;
    const float paddedHeight = height * 1.10f;

    const float zoomX = static_cast<float>(m_clientSize.width) / paddedWidth;
    const float zoomY = static_cast<float>(m_clientSize.height) / paddedHeight;

    m_panZoom.pan = glm::vec2(0.0f, 0.0f);
    m_panZoom.zoom = std::clamp(std::min(zoomX, zoomY), m_panZoom.minZoom, m_panZoom.maxZoom);
    m_needsFitToClient = false;
}

float Application::SnapScalarToGrid(float value, float spacing)
{
    if (spacing <= 1.0e-6f)
        return value;

    return std::round(value / spacing) * spacing;
}

glm::vec2 Application::SnapWorldToGrid(const glm::vec2& world) const
{
    return glm::vec2(
        SnapScalarToGrid(world.x, m_gridSpacingWorld),
        SnapScalarToGrid(world.y, m_gridSpacingWorld));
}

void Application::RefreshSnappedMousePosition()
{
    if (!m_clientSize.IsValid())
    {
        m_crosshairs.SetMouseClient(0, 0);
        return;
    }

    int snappedX = std::clamp(m_rawMouseX, 0, std::max(0, m_clientSize.width - 1));
    int snappedY = std::clamp(m_rawMouseY, 0, std::max(0, m_clientSize.height - 1));

    if (m_gridSnapEnabled && !m_selection.IsBoxSelectionActive() && !m_panWithRightDrag)
    {
        const glm::vec2 snappedWorld = SnapWorldToGrid(m_panZoom.ScreenToWorld(m_rawMouseX, m_rawMouseY));
        const glm::vec2 snappedScreen = m_panZoom.WorldToScreen(snappedWorld.x, snappedWorld.y);
        snappedX = std::clamp(static_cast<int>(std::lround(snappedScreen.x)), 0, std::max(0, m_clientSize.width - 1));
        snappedY = std::clamp(static_cast<int>(std::lround(snappedScreen.y)), 0, std::max(0, m_clientSize.height - 1));
    }

    m_crosshairs.SetMouseClient(snappedX, snappedY);
}


float Application::DistancePointToSegmentPx(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b, float& outT)
{
    const glm::vec2 ab = b - a;
    const float abLen2 = glm::dot(ab, ab);
    if (abLen2 <= 1.0e-6f)
    {
        outT = 0.0f;
        return glm::length(p - a);
    }

    outT = glm::clamp(glm::dot(p - a, ab) / abLen2, 0.0f, 1.0f);
    const glm::vec2 closest = a + (ab * outT);
    return glm::length(p - closest);
}

bool Application::SegmentIntersectsAabb2D(const glm::vec2& a, const glm::vec2& b,
                                          const glm::vec2& mn, const glm::vec2& mx)
{
    glm::vec2 smin(std::min(a.x, b.x), std::min(a.y, b.y));
    glm::vec2 smax(std::max(a.x, b.x), std::max(a.y, b.y));
    if (smax.x < mn.x || smin.x > mx.x || smax.y < mn.y || smin.y > mx.y)
        return false;

    glm::vec2 d = b - a;
    float t0 = 0.0f;
    float t1 = 1.0f;

    auto clip = [&](float p, float q) -> bool
    {
        if (std::abs(p) < 1.0e-8f)
            return q >= 0.0f;
        const float r = q / p;
        if (p < 0.0f)
        {
            if (r > t1) return false;
            if (r > t0) t0 = r;
        }
        else
        {
            if (r < t0) return false;
            if (r < t1) t1 = r;
        }
        return true;
    };

    return clip(-d.x, a.x - mn.x) && clip(d.x, mx.x - a.x) && clip(-d.y, a.y - mn.y) && clip(d.y, mx.y - a.y);
}

float Application::DistanceClientPx(int x0, int y0, int x1, int y1)
{
    const float dx = static_cast<float>(x1 - x0);
    const float dy = static_cast<float>(y1 - y0);
    return std::sqrt((dx * dx) + (dy * dy));
}

SelectionHit Application::FindBestEntityHitAtClient(int clientX, int clientY) const
{
    SelectionHit best{};

    if (!m_book || !m_clientSize.IsValid())
        return best;

    const glm::vec2 mousePx(
        static_cast<float>(std::clamp(clientX, 0, std::max(0, m_clientSize.width - 1))),
        static_cast<float>(std::clamp(clientY, 0, std::max(0, m_clientSize.height - 1))));

    float bestDistance = m_hoverPickRadiusPx;

    for (const auto& ep : m_book->entities)
    {
        if (!ep || ep->type != EntityType::Line)
            continue;

        const auto* line = ep->As<LineEntity>();
        if (!line || !line->visible || !m_layerTable.IsLayerSelectable(line->layerId) || !line->pickable || line->tag != EntityTag::Scene)
            continue;

        const glm::vec2 a = m_panZoom.WorldToScreen(line->p0.x, line->p0.y);
        const glm::vec2 b = m_panZoom.WorldToScreen(line->p1.x, line->p1.y);

        float t = 0.0f;
        const float dist = DistancePointToSegmentPx(mousePx, a, b, t);
        if (dist <= bestDistance)
        {
            bestDistance = dist;
            best.id = line->ID;
            best.score = dist;
        }
    }

    return best;
}

void Application::UpdateHoveredEntity()
{
    if (!m_book || !m_mouseInsideClient || !m_clientSize.IsValid() || m_selection.IsBoxSelectionActive())
    {
        m_selection.ClearHover();
        return;
    }

    m_selection.SetHoverHit(FindBestEntityHitAtClient(m_rawMouseX, m_rawMouseY));
}


void Application::AppendGlowOverlayForLine(const LineEntity& line, const glm::vec4& color, float offsetPx)
{
    if (!line.visible || !m_layerTable.IsLayerVisible(line.layerId))
        return;

    const glm::vec2 a = m_panZoom.WorldToScreen(line.p0.x, line.p0.y);
    const glm::vec2 b = m_panZoom.WorldToScreen(line.p1.x, line.p1.y);

    m_overlayVertices.push_back({ glm::vec3(a, 0.0f), color });
    m_overlayVertices.push_back({ glm::vec3(b, 0.0f), color });

    const glm::vec2 d = b - a;
    const float len = glm::length(d);
    if (len > 1.0e-3f)
    {
        const glm::vec2 n(-d.y / len, d.x / len);
        const glm::vec2 off = n * offsetPx;
        m_overlayVertices.push_back({ glm::vec3(a + off, 0.0f), color });
        m_overlayVertices.push_back({ glm::vec3(b + off, 0.0f), color });
        m_overlayVertices.push_back({ glm::vec3(a - off, 0.0f), color });
        m_overlayVertices.push_back({ glm::vec3(b - off, 0.0f), color });
    }
}

void Application::AppendGlowOverlayForEntity(std::size_t entityId, const glm::vec4& color, float offsetPx)
{
    const LineEntity* line = FindLineById(entityId);
    if (line)
        AppendGlowOverlayForLine(*line, color, offsetPx);
}


void Application::AppendGripOverlayForLine(const LineEntity& line, const glm::vec4& color, float halfSizePx)
{
    if (!line.visible || !m_layerTable.IsLayerVisible(line.layerId))
        return;

    auto appendSquare = [&](const glm::vec2& center)
    {
        const glm::vec3 p0(center.x - halfSizePx, center.y - halfSizePx, 0.0f);
        const glm::vec3 p1(center.x + halfSizePx, center.y - halfSizePx, 0.0f);
        const glm::vec3 p2(center.x + halfSizePx, center.y + halfSizePx, 0.0f);
        const glm::vec3 p3(center.x - halfSizePx, center.y + halfSizePx, 0.0f);
        m_overlayVertices.push_back({ p0, color }); m_overlayVertices.push_back({ p1, color });
        m_overlayVertices.push_back({ p1, color }); m_overlayVertices.push_back({ p2, color });
        m_overlayVertices.push_back({ p2, color }); m_overlayVertices.push_back({ p3, color });
        m_overlayVertices.push_back({ p3, color }); m_overlayVertices.push_back({ p0, color });
    };

    appendSquare(m_panZoom.WorldToScreen(line.p0.x, line.p0.y));
    appendSquare(m_panZoom.WorldToScreen(line.p1.x, line.p1.y));
}

void Application::AppendGripOverlayForEntity(std::size_t entityId, const glm::vec4& color, float halfSizePx)
{
    const LineEntity* line = FindLineById(entityId);
    if (line)
        AppendGripOverlayForLine(*line, color, halfSizePx);
}

void Application::AppendBoxSelectionOverlay()
{
    if (!m_selection.IsBoxSelectionActive())
        return;

    const BoxSelectionState& box = m_selection.GetBoxSelectionState();
    const glm::vec2 a = m_panZoom.WorldToScreen(box.startWorld.x, box.startWorld.y);
    const glm::vec2 b = m_panZoom.WorldToScreen(box.currentWorld.x, box.currentWorld.y);
    const float x0 = std::min(a.x, b.x);
    const float x1 = std::max(a.x, b.x);
    const float y0 = std::min(a.y, b.y);
    const float y1 = std::max(a.y, b.y);
    const glm::vec4 color = box.crossing ? m_crossingBoxColor : m_windowBoxColor;

    const glm::vec3 p0(x0, y0, 0.0f);
    const glm::vec3 p1(x1, y0, 0.0f);
    const glm::vec3 p2(x1, y1, 0.0f);
    const glm::vec3 p3(x0, y1, 0.0f);
    m_overlayVertices.push_back({ p0, color }); m_overlayVertices.push_back({ p1, color });
    m_overlayVertices.push_back({ p1, color }); m_overlayVertices.push_back({ p2, color });
    m_overlayVertices.push_back({ p2, color }); m_overlayVertices.push_back({ p3, color });
    m_overlayVertices.push_back({ p3, color }); m_overlayVertices.push_back({ p0, color });
}

void Application::ApplyActiveBoxSelection()
{
    if (!m_book || !m_selection.IsBoxSelectionActive())
        return;

    const BoxSelectionState& box = m_selection.GetBoxSelectionState();
    const glm::vec2 worldMin(glm::min(box.startWorld.x, box.currentWorld.x), glm::min(box.startWorld.y, box.currentWorld.y));
    const glm::vec2 worldMax(glm::max(box.startWorld.x, box.currentWorld.x), glm::max(box.startWorld.y, box.currentWorld.y));

    for (const auto& ep : m_book->entities)
    {
        if (!ep || ep->type != EntityType::Line)
            continue;

        const auto* line = ep->As<LineEntity>();
        if (!line || !line->visible || !m_layerTable.IsLayerSelectable(line->layerId) || !line->pickable || line->tag != EntityTag::Scene)
            continue;

        const glm::vec2 a(line->p0.x, line->p0.y);
        const glm::vec2 b(line->p1.x, line->p1.y);

        bool include = false;
        if (box.crossing)
        {
            include = SegmentIntersectsAabb2D(a, b, worldMin, worldMax);
        }
        else
        {
            const bool aInside = (a.x >= worldMin.x && a.x <= worldMax.x && a.y >= worldMin.y && a.y <= worldMax.y);
            const bool bInside = (b.x >= worldMin.x && b.x <= worldMax.x && b.y >= worldMin.y && b.y <= worldMax.y);
            include = aInside && bInside;
        }

        if (include)
            m_selection.Add(line->ID);
    }
}


bool Application::IsCtrlDown() const
{
    return (GetKeyState(VK_CONTROL) & 0x8000) != 0;
}

void Application::SyncPropertiesWindow()
{
    if (m_propertiesWindow.IsVisible())
        m_propertiesWindow.RefreshFromHost();
}

std::string Application::BuildEntityDisplayName(const Entity& entity) const
{
    std::ostringstream ss;
    switch (entity.type)
    {
    case EntityType::Line: ss << "Line"; break;
    case EntityType::Circle: ss << "Circle"; break;
    case EntityType::Polyline: ss << "Polyline"; break;
    default: ss << "Entity"; break;
    }
    ss << ' ' << entity.ID;
    return ss.str();
}

std::string Application::BuildColorName(const glm::vec4& color) const
{
    auto nearEq = [](float a, float b) { return std::abs(a - b) <= 0.05f; };
    if (nearEq(color.r, 1.0f) && nearEq(color.g, 0.0f) && nearEq(color.b, 0.0f)) return "Red";
    if (nearEq(color.r, 0.0f) && nearEq(color.g, 1.0f) && nearEq(color.b, 0.0f)) return "Green";
    if (nearEq(color.r, 0.0f) && nearEq(color.g, 0.0f) && nearEq(color.b, 1.0f)) return "Blue";
    if (nearEq(color.r, 1.0f) && nearEq(color.g, 1.0f) && nearEq(color.b, 0.0f)) return "Yellow";

    std::ostringstream ss;
    ss << "RGB(" << int(color.r * 255.0f + 0.5f) << ',' << int(color.g * 255.0f + 0.5f) << ',' << int(color.b * 255.0f + 0.5f) << ')';
    return ss.str();
}


const LineEntity* Application::FindLineById(std::size_t entityId) const
{
    if (entityId == 0)
        return nullptr;

    const auto it = m_lineEntityIndex.find(entityId);
    return (it != m_lineEntityIndex.end()) ? it->second : nullptr;
}

LineEntity* Application::FindLineById(std::size_t entityId)
{
    return const_cast<LineEntity*>(static_cast<const Application*>(this)->FindLineById(entityId));
}

std::vector<SelectedEntityInfo> Application::BuildSelectedEntities() const
{
    std::vector<SelectedEntityInfo> out;
    out.reserve(m_selection.GetSelectionCount());

    if (!m_book)
        return out;

    for (std::size_t id : m_selection.GetSelectedEntities())
    {
        const LineEntity* line = FindLineById(id);
        if (!line)
            continue;

        SelectedEntityInfo info{};
        info.entityId = id;
        info.kind = EntityKind::Line;
        info.displayName = BuildEntityDisplayName(*line);
        if (const LayerTable::Layer* layer = m_layerTable.Find(line->layerId))
            info.layerName = layer->name;
        else
            info.layerName = "Default";
        info.colorName = BuildColorName(line->color);
        out.push_back(std::move(info));
    }

    return out;
}

bool Application::TryGetSingleSelectedLine(SingleLineSelectionInfo& outInfo) const
{
    if (m_selection.GetSelectionCount() != 1)
        return false;

    const std::size_t id = *m_selection.GetSelectedEntities().begin();
    const LineEntity* line = FindLineById(id);
    if (!line)
        return false;

    outInfo.entityId = id;
    outInfo.screenSpace = line->screenSpace;
    outInfo.startX = line->p0.x;
    outInfo.startY = line->p0.y;
    outInfo.startZ = line->p0.z;
    outInfo.endX = line->p1.x;
    outInfo.endY = line->p1.y;
    outInfo.endZ = line->p1.z;
    return true;
}

bool Application::UpdateSingleSelectedLine(const SingleLineSelectionInfo& info)
{
    LineEntity* line = FindLineById(info.entityId);
    if (!line)
        return false;

    line->p0 = glm::vec3(info.startX, info.startY, info.startZ);
    line->p1 = glm::vec3(info.endX, info.endY, info.endZ);
    UploadSceneToGpu();
    return true;
}

std::vector<std::pair<std::string, std::uint32_t>> Application::BuildLayerChoices() const
{
    std::vector<std::pair<std::string, std::uint32_t>> out;
    for (const auto& layer : m_layerTable.GetLayers())
        out.emplace_back(layer.name, layer.id);
    return out;
}

bool Application::AssignSelectionLayer(std::uint32_t layerId, EntityKind filterKind)
{
    bool changed = false;
    for (std::size_t id : m_selection.GetSelectedEntities())
    {
        LineEntity* line = FindLineById(id);
        if (!line)
            continue;
        if (filterKind != EntityKind::Unknown && filterKind != EntityKind::Line)
            continue;
        line->layerId = layerId;
        changed = true;
    }
    if (changed)
        UploadSceneToGpu();
    return changed;
}
