#pragma once

#include <glad/glad.h>
#include <string>

namespace GLShaderUtil
{
    GLuint create(const std::string& vertexSource,
        const std::string& fragmentSource,
        const std::string& geometrySource = "");
}
