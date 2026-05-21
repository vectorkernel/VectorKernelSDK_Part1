#include "Application.h"

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

    BuildDragonScene();
    ResetView();

    std::cout << "Snap grid demo controls:\n"
              << "  F9 : toggle snap-to-grid\n"
              << "  X  : cycle crosshair mode\n"
              << "  B  : cycle background preset\n"
              << "  R  : reset view\n"
              << "  RMB drag : pan\n"
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
        line->pickable = false;
        line->visible = true;
        line->selected = false;
        line->p0 = glm::vec3(segment.a, 0.0f);
        line->p1 = glm::vec3(segment.b, 0.0f);
        line->color = segment.color;
        line->width = 1.0f;
        m_book->entities.push_back(std::move(line));
    }

    m_book->SortByDrawOrder();
    UploadSceneToGpu();
    m_needsFitToClient = true;

    std::cout << "Dragon built: " << m_dragon.segments.size()
              << " segments across " << kColors.size()
              << " Rosetta passes using 1 worker(s).\n";
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
    RebuildOverlayLines();
    UploadOverlayToGpu();
}

void Application::OnRMouseButtonDown(int x, int y)
{
    m_rightButtonPanning = true;
    m_rawMouseX = x;
    m_rawMouseY = y;
    m_needsFitToClient = false;
    m_panZoom.OnMouseDown(x, y);
    RefreshSnappedMousePosition();
    RebuildOverlayLines();
    UploadOverlayToGpu();
}

void Application::OnRMouseButtonUp()
{
    m_rightButtonPanning = false;
    m_panZoom.OnMouseUp();
    RefreshSnappedMousePosition();
    RebuildOverlayLines();
    UploadOverlayToGpu();
}

void Application::OnMouseMove(int x, int y, bool rightButtonDown)
{
    m_mouseInsideClient = true;
    m_rawMouseX = x;
    m_rawMouseY = y;

    if (rightButtonDown)
    {
        m_needsFitToClient = false;
        m_panZoom.OnMouseMove(x, y);
        RebuildGridIfNeeded();
    }

    m_rightButtonPanning = rightButtonDown;
    RefreshSnappedMousePosition();
    RebuildOverlayLines();
    UploadOverlayToGpu();
}

void Application::OnMouseLeave()
{
    m_mouseInsideClient = false;
    RebuildOverlayLines();
    UploadOverlayToGpu();
}

void Application::OnMouseWheel(int x, int y, int wheelDelta)
{
    m_needsFitToClient = false;
    m_panZoom.OnMouseWheel(x, y, wheelDelta);
    RebuildGridIfNeeded();
    RefreshSnappedMousePosition();
    RebuildOverlayLines();
    UploadOverlayToGpu();
}

void Application::OnKeyDown(unsigned int vk)
{
    switch (vk)
    {
    case VK_F9:
        m_gridSnapEnabled = !m_gridSnapEnabled;
        RefreshSnappedMousePosition();
        RebuildOverlayLines();
        UploadOverlayToGpu();
        std::cout << "Grid snap: " << (m_gridSnapEnabled ? "ON" : "OFF") << "\n";
        break;

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

    case 'R':
        ResetView();
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

    if (!m_mouseInsideClient)
        return;

    std::vector<LineEntity> crosshairLines;
    m_crosshairs.BuildLines(crosshairLines);

    m_overlayVertices.reserve(crosshairLines.size() * 2);

    for (const LineEntity& line : crosshairLines)
    {
        if (!line.visible)
            continue;

        m_overlayVertices.push_back({ line.p0, line.color });
        m_overlayVertices.push_back({ line.p1, line.color });
    }
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
        if (!line || !line->visible)
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

    if (m_gridSnapEnabled && !m_rightButtonPanning)
    {
        const glm::vec2 snappedWorld = SnapWorldToGrid(m_panZoom.ScreenToWorld(m_rawMouseX, m_rawMouseY));
        const glm::vec2 snappedScreen = m_panZoom.WorldToScreen(snappedWorld.x, snappedWorld.y);
        snappedX = std::clamp(static_cast<int>(std::lround(snappedScreen.x)), 0, std::max(0, m_clientSize.width - 1));
        snappedY = std::clamp(static_cast<int>(std::lround(snappedScreen.y)), 0, std::max(0, m_clientSize.height - 1));
    }

    m_crosshairs.SetMouseClient(snappedX, snappedY);
}
