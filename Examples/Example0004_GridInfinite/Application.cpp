#include "Application.h"

#include <iostream>
#include <cstddef>
#include <utility>

#include "..\..\AppCoreLib\Platform\ConsoleLogSink.h"

#include <glm/gtc/type_ptr.hpp>

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
    glGenBuffers(1, &m_vbo);

    // These are the core camera settings that make the new project behave like
    // an interactive CAD-style grid demo rather than the fixed-view OpenGL test.
    m_panZoom.mode = PanZoomController::OrthoMode::Center;
    m_panZoom.minZoom = 0.02f;
    m_panZoom.maxZoom = 100.0f;
    m_panZoom.wheelZoomFactor = 1.12f;
    m_panZoom.dragSpeed = 1.0f;

    ResetView();
}

void Application::Update(float /*inDeltaTime*/)
{
    // The grid is cached, so this call is cheap when the visible cell range has not changed.
    RebuildGridIfNeeded();
}

void Application::Render(const ClientSize& clientSize)
{
    if (!m_book || !clientSize.IsValid())
        return;

    if (clientSize != m_clientSize)
        OnResize(clientSize);

    glViewport(0, 0, m_clientSize.width, m_clientSize.height);
    glClearColor(0.10f, 0.11f, 0.13f, 1.0f);
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
    RebuildGridIfNeeded();
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

    m_panZoom.OnMouseMove(x, y);
    RebuildGridIfNeeded();
}

void Application::OnMouseWheel(int x, int y, int wheelDelta)
{
    m_panZoom.OnMouseWheel(x, y, wheelDelta);
    RebuildGridIfNeeded();
}

void Application::ResetView()
{
    m_panZoom.pan = { 0.0f, 0.0f };
    m_panZoom.zoom = 1.0f;
    m_gridCache = {};
    RebuildGridIfNeeded();
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
        UploadLinesToGpu();

    return rebuilt;
}

void Application::UploadLinesToGpu()
{
    if (!m_book)
        return;

    m_vertices.clear();
    m_vertices.reserve(m_book->entities.size() * 2);

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
        m_vertices.data(),
        GL_STATIC_DRAW);

    std::cout << "Uploaded " << (m_vertices.size() / 2) << " line segments\n";
}
