#include "Application.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <utility>

#include <glm/gtc/type_ptr.hpp>

#include "..\..\RenderCoreLib\GLShaderUtil.h"

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

    EntityBookCreateResult created = CreateBook(300000);
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
    glGenBuffers(1, &m_vbo);

    m_panZoom.mode = PanZoomController::OrthoMode::Center;
    m_panZoom.minZoom = 0.01f;
    m_panZoom.maxZoom = 200.0f;
    m_panZoom.wheelZoomFactor = 1.12f;
    m_panZoom.dragSpeed = 1.0f;

    BuildDragonScene();
}

void Application::BuildDragonScene()
{
    if (!m_book)
        return;

    m_book->Clear();
    m_nextId = 1;

    // Match Example0001_RosettaDragon exactly.
    // That reference does NOT draw an initial segment before processing the generator.
    // Instead, each pass starts from a slightly shifted MoveTo point, turns first,
    // and then draws one segment per generator element while carrying the heading
    // forward across all four passes.
    constexpr int kOrder = 17;
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
        glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), // red
        glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), // green
        glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), // blue
        glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), // yellow
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

    m_book->entities.reserve(m_dragon.segments.size());
    for (const RenderCore::DragonSegment& segment : m_dragon.segments)
    {
        auto line = std::make_unique<LineEntity>();
        line->ID = m_nextId++;
        line->p0 = glm::vec3(segment.a, 0.0f);
        line->p1 = glm::vec3(segment.b, 0.0f);
        line->color = segment.color;
        line->width = 1.0f;
        m_book->entities.push_back(std::move(line));
    }

    std::cout << "Dragon built: " << m_dragon.segments.size()
              << " segments across " << kColors.size()
              << " Rosetta passes using 1 worker(s).\n";

    UploadLinesToGpu();
    m_needsFitToClient = true;
    FitDragonToClient();
}

void Application::Update(float /*inDeltaTime*/)
{
}

void Application::Render(const ClientSize& clientSize)
{
    if (!m_book || !clientSize.IsValid() || m_program == 0)
        return;

    if (clientSize != m_clientSize)
        OnResize(clientSize);

    glViewport(0, 0, m_clientSize.width, m_clientSize.height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_program);

    const glm::mat4 mvp = m_panZoom.GetMVP();
    glUniformMatrix4fv(m_uMvp, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));

    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
}

void Application::Shutdown()
{
    if (m_program == 0 && m_vbo == 0 && !m_book)
        return;

    std::cout << "Application::Shutdown\n";

    EntityCore::SetLogSink(nullptr);

    if (m_vbo)
    {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }

    if (m_program)
    {
        glDeleteProgram(m_program);
        m_program = 0;
    }

    m_vertices.clear();

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
    FitDragonToClient();
}

void Application::OnRMouseButtonDown(int x, int y)
{
    m_panZoom.OnMouseDown(x, y);
}

void Application::OnRMouseButtonUp()
{
    m_panZoom.OnMouseUp();
}

void Application::OnMouseMove(int x, int y, bool rightButtonDown)
{
    if (!rightButtonDown)
        return;

    m_needsFitToClient = false;
    m_panZoom.OnMouseMove(x, y);
}

void Application::OnMouseWheel(int x, int y, int wheelDelta)
{
    m_needsFitToClient = false;
    m_panZoom.OnMouseWheel(x, y, wheelDelta);
}

void Application::ResetView()
{
    m_needsFitToClient = true;
    FitDragonToClient();
}

void Application::UploadLinesToGpu()
{
    if (!m_book)
        return;

    m_vertices.clear();
    m_vertices.reserve(m_book->entities.size() * 2u);

    for (const auto& ep : m_book->entities)
    {
        if (!ep || ep->type != EntityType::Line)
            continue;

        const auto* line = ep->As<LineEntity>();
        if (!line || !line->visible)
            continue;

        m_vertices.push_back({ line->p0, line->color });
        m_vertices.push_back({ line->p1, line->color });
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)),
                 m_vertices.empty() ? nullptr : m_vertices.data(),
                 GL_STATIC_DRAW);
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
