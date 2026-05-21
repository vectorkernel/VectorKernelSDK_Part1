#include "Application.h"

#include <iostream>
#include <cstddef>
#include <utility>

#include "..\..\AppCoreLib\Platform\ConsoleLogSink.h"

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

void Application::Initialize()
{
    AppCore::InstallConsoleLogSink();
    std::cout << "Application::Initialize\n";

    EntityBookCreateResult created = CreateBook(32768);
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

    ResetView();
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
    RebuildGridIfNeeded();
    RebuildOverlayLines();
    UploadOverlayToGpu();
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
    m_mouseInsideClient = true;
    m_crosshairs.SetMouseClient(x, y);

    if (rightButtonDown)
    {
        m_panZoom.OnMouseMove(x, y);
        RebuildGridIfNeeded();
    }

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
    m_panZoom.OnMouseWheel(x, y, wheelDelta);
    RebuildGridIfNeeded();
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
    m_gridCache = {};
    RebuildGridIfNeeded();
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
        m_sceneVertices.data(),
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
