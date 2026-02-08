#pragma once

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace asset
{
    class Shader
    {
    public:
        Shader() = default;
        ~Shader();

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        Shader(Shader&& other) noexcept;
        Shader& operator=(Shader&& other) noexcept;

        bool LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
        void Use() const;

        void SetMat4(const std::string& name, const glm::mat4& value);
        void SetVec3(const std::string& name, const glm::vec3& value);
        void SetVec4(const std::string& name, const glm::vec4& value);
        void SetFloat(const std::string& name, float value);
        void SetInt(const std::string& name, int value);

        unsigned int ProgramId() const;

    private:
        int GetUniformLocation(const std::string& name);

        static std::string ReadTextFile(const std::string& path);
        static unsigned int CompileStage(unsigned int stage, const std::string& source, std::string& outError);
        static unsigned int LinkProgram(unsigned int vs, unsigned int fs, std::string& outError);

    private:
        unsigned int m_program{0};
        std::unordered_map<std::string, int> m_uniformCache;
    };
} // namespace gfx
