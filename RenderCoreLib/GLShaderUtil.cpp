#include "pch.h"
#include "GLShaderUtil.h"

#include <iostream>

namespace GLShaderUtil
{
    GLuint create(const std::string& vertexSource,
        const std::string& fragmentSource,
        const std::string& geometrySource)
    {
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        const char* vertexSrc = vertexSource.c_str();
        glShaderSource(vertexShader, 1, &vertexSrc, nullptr);
        glCompileShader(vertexShader);

        GLint success = GL_FALSE;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512]{};
            glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
            std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        const char* fragmentSrc = fragmentSource.c_str();
        glShaderSource(fragmentShader, 1, &fragmentSrc, nullptr);
        glCompileShader(fragmentShader);

        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512]{};
            glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
            std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        GLuint geometryShader = 0;
        if (!geometrySource.empty())
        {
            geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
            const char* geometrySrc = geometrySource.c_str();
            glShaderSource(geometryShader, 1, &geometrySrc, nullptr);
            glCompileShader(geometryShader);

            glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                char infoLog[512]{};
                glGetShaderInfoLog(geometryShader, 512, nullptr, infoLog);
                std::cerr << "ERROR::SHADER::GEOMETRY::COMPILATION_FAILED\n" << infoLog << std::endl;
            }
        }

        GLuint shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        if (geometryShader)
            glAttachShader(shaderProgram, geometryShader);
        glLinkProgram(shaderProgram);

        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success)
        {
            char infoLog[512]{};
            glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        if (geometryShader)
            glDeleteShader(geometryShader);

        return shaderProgram;
    }
}