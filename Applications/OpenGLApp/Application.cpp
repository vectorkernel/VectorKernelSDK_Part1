#include "Application.h"

#include <iostream>
#include <cmath>
#include <cstddef>
#include <utility>

#include "..\RenderCoreLib\GLShaderUtil.h"

namespace
{
    constexpr float kGridSpacing = 1.0f;
    constexpr float kGridMajorEvery = 5.0f;
    constexpr float kGridEpsilon = 0.001f;
    constexpr float kDefaultViewHalfHeight = 10.0f;
}

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

    const char* cat = category ? category : "(null)";
    const char* evt = event ? event : "(null)";

    std::cout << "[" << lvl << "] " << cat << "::" << evt;
    if (!message.empty())
        std::cout << " | " << message;
    std::cout << "\n";
}

Application::Mat4 Application::BuildOrtho(float left, float right, float bottom, float top, float nearZ, float farZ)
{
    Mat4 result{};
    result.m[0] = 2.0f / (right - left);
    result.m[5] = 2.0f / (top - bottom);
    result.m[10] = -2.0f / (farZ - nearZ);
    result.m[12] = -(right + left) / (right - left);
    result.m[13] = -(top + bottom) / (top - bottom);
    result.m[14] = -(farZ + nearZ) / (farZ - nearZ);
    result.m[15] = 1.0f;
    return result;
}

void Application::Initialize()
{
    EntityCore::SetLogSink(&Application::LogSink);
    std::cout << "Application::Initialize\n";

    EntityBookCreateResult created = CreateBook(8192);
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

    m_viewHalfHeight = kDefaultViewHalfHeight;
    m_viewHalfWidth = kDefaultViewHalfHeight;
    BuildGrid(m_viewHalfWidth, m_viewHalfHeight);
    UploadLinesToGpu();
}

void Application::Update(float /*inDeltaTime*/)
{
}

void Application::Render(const ClientSize& clientSize)
{
    if (!clientSize.IsValid())
        return;

    const int w = clientSize.width;
    const int h = clientSize.height;

    const float aspect = clientSize.AspectRatioOr();
    const float viewHalfHeight = kDefaultViewHalfHeight;
    const float viewHalfWidth = viewHalfHeight * aspect;

    if (clientSize != m_lastClientSize ||
        std::abs(viewHalfWidth - m_viewHalfWidth) > kGridEpsilon ||
        std::abs(viewHalfHeight - m_viewHalfHeight) > kGridEpsilon)
    {
        m_lastClientSize = clientSize;
        m_viewHalfWidth = viewHalfWidth;
        m_viewHalfHeight = viewHalfHeight;

        BuildGrid(m_viewHalfWidth, m_viewHalfHeight);
        UploadLinesToGpu();
    }

    glViewport(0, 0, w, h);
    glClearColor(0.10f, 0.11f, 0.13f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_program);

    const Mat4 proj = BuildOrtho(
        -m_viewHalfWidth, m_viewHalfWidth,
        -m_viewHalfHeight, m_viewHalfHeight,
        -1.0f, 1.0f);

    glUniformMatrix4fv(m_uMvp, 1, GL_FALSE, proj.m);

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

void Application::BuildGrid(float modelHalfW, float modelHalfH)
{
    if (!m_book)
        return;

    m_book->Clear();

    auto addLine = [&](const Vec3& a, const Vec3& b, const Vec4& color)
    {
        auto e = std::make_unique<LineEntity>();
        e->ID = m_nextId++;
        e->tag = EntityTag::Scene;
        e->drawOrder = 0;
        e->screenSpace = false;
        e->visible = true;
        e->pickable = false;
        e->p0 = a;
        e->p1 = b;
        e->color = color;
        e->width = 1.0f;

        m_book->AddEntity(std::move(e));
    };

    for (float x = -modelHalfW; x <= modelHalfW + kGridEpsilon; x += kGridSpacing)
    {
        const bool major = std::fmod(std::abs(x), kGridMajorEvery) < kGridEpsilon;
        const Vec4 c = major ? Vec4(0.30f, 0.32f, 0.36f, 1.0f)
                             : Vec4(0.20f, 0.22f, 0.26f, 1.0f);

        addLine(Vec3(x, -modelHalfH, 0.0f), Vec3(x, modelHalfH, 0.0f), c);
    }

    for (float y = -modelHalfH; y <= modelHalfH + kGridEpsilon; y += kGridSpacing)
    {
        const bool major = std::fmod(std::abs(y), kGridMajorEvery) < kGridEpsilon;
        const Vec4 c = major ? Vec4(0.30f, 0.32f, 0.36f, 1.0f)
                             : Vec4(0.20f, 0.22f, 0.26f, 1.0f);

        addLine(Vec3(-modelHalfW, y, 0.0f), Vec3(modelHalfW, y, 0.0f), c);
    }

    addLine(Vec3(-modelHalfW, 0.0f, 0.0f), Vec3(modelHalfW, 0.0f, 0.0f), Vec4(0.70f, 0.20f, 0.20f, 1.0f));
    addLine(Vec3(0.0f, -modelHalfH, 0.0f), Vec3(0.0f, modelHalfH, 0.0f), Vec4(0.20f, 0.70f, 0.20f, 1.0f));

    m_book->SortByDrawOrder();
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

        const auto& line = static_cast<const LineEntity&>(*ep);
        m_vertices.push_back({ line.p0, line.color });
        m_vertices.push_back({ line.p1, line.color });
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)),
        m_vertices.data(),
        GL_STATIC_DRAW);

    std::cout << "Uploaded " << (m_vertices.size() / 2) << " line segments\n";
}
