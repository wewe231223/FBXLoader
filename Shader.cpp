#include "Shader.h"

#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <vector>

namespace asset
{
    Shader::~Shader()
    {
        if (m_program != 0)
        {
            glDeleteProgram(m_program);
            m_program = 0;
        }
    }

    Shader::Shader(Shader&& other) noexcept
        : m_program(other.m_program)
        , m_uniformCache(std::move(other.m_uniformCache))
    {
        other.m_program = 0;
    }

    Shader& Shader::operator=(Shader&& other) noexcept
    {
        if (this != &other)
        {
            if (m_program != 0)
            {
                glDeleteProgram(m_program);
            }
            m_program = other.m_program;
            m_uniformCache = std::move(other.m_uniformCache);
            other.m_program = 0;
        }
        return *this;
    }

    unsigned int Shader::ProgramId() const
    {
        return m_program;
    }

    bool Shader::LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath)
    {
        const std::string vsSrc = ReadTextFile(vertexPath);
        const std::string fsSrc = ReadTextFile(fragmentPath);

        if (vsSrc.empty() || fsSrc.empty())
        {
            return false;
        }

        std::string err;
        const unsigned int vs = CompileStage(GL_VERTEX_SHADER, vsSrc, err);
        if (vs == 0)
        {
            return false;
        }

        const unsigned int fs = CompileStage(GL_FRAGMENT_SHADER, fsSrc, err);
        if (fs == 0)
        {
            glDeleteShader(vs);
            return false;
        }

        const unsigned int program = LinkProgram(vs, fs, err);

        glDeleteShader(vs);
        glDeleteShader(fs);

        if (program == 0)
        {
            return false;
        }

        if (m_program != 0)
        {
            glDeleteProgram(m_program);
        }

        m_program = program;
        m_uniformCache.clear();
        return true;
    }

    void Shader::Use() const
    {
        glUseProgram(m_program);
    }

    void Shader::SetMat4(const std::string& name, const glm::mat4& value)
    {
        const int loc = GetUniformLocation(name);
        if (loc >= 0)
        {
            glUniformMatrix4fv(loc, 1, GL_FALSE, &value[0][0]);
        }
    }

    void Shader::SetVec3(const std::string& name, const glm::vec3& value)
    {
        const int loc = GetUniformLocation(name);
        if (loc >= 0)
        {
            glUniform3fv(loc, 1, &value[0]);
        }
    }

    void Shader::SetVec4(const std::string& name, const glm::vec4& value)
    {
        const int loc = GetUniformLocation(name);
        if (loc >= 0)
        {
            glUniform4fv(loc, 1, &value[0]);
        }
    }

    void Shader::SetFloat(const std::string& name, float value)
    {
        const int loc = GetUniformLocation(name);
        if (loc >= 0)
        {
            glUniform1f(loc, value);
        }
    }

    void Shader::SetInt(const std::string& name, int value)
    {
        const int loc = GetUniformLocation(name);
        if (loc >= 0)
        {
            glUniform1i(loc, value);
        }
    }

    int Shader::GetUniformLocation(const std::string& name)
    {
        const auto it = m_uniformCache.find(name);
        if (it != m_uniformCache.end())
        {
            return it->second;
        }

        const int loc = glGetUniformLocation(m_program, name.c_str());
        m_uniformCache.emplace(name, loc);
        return loc;
    }

    std::string Shader::ReadTextFile(const std::string& path)
    {
        std::ifstream ifs(path, std::ios::in);
        if (!ifs)
        {
            return {};
        }
        std::ostringstream oss;
        oss << ifs.rdbuf();
        return oss.str();
    }

    unsigned int Shader::CompileStage(unsigned int stage, const std::string& source, std::string& outError)
    {
        const unsigned int id = glCreateShader(stage);
        const char* src = source.c_str();
        glShaderSource(id, 1, &src, nullptr);
        glCompileShader(id);

        int ok = 0;
        glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
        if (ok == 0)
        {
            int len = 0;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
            std::vector<char> buf(static_cast<size_t>(len) + 1);
            glGetShaderInfoLog(id, len, nullptr, buf.data());
            outError.assign(buf.data());

            glDeleteShader(id);
            return 0;
        }
        return id;
    }

    unsigned int Shader::LinkProgram(unsigned int vs, unsigned int fs, std::string& outError)
    {
        const unsigned int program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        int ok = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &ok);
        if (ok == 0)
        {
            int len = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
            std::vector<char> buf(static_cast<size_t>(len) + 1);
            glGetProgramInfoLog(program, len, nullptr, buf.data());
            outError.assign(buf.data());

            glDeleteProgram(program);
            return 0;
        }
        return program;
    }
} // namespace gfx
