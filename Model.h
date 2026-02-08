#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include <glad/glad.h>

#include "Common.h"

namespace asset
{
    class Model
    {
    public:
        Model() = default;
        ~Model();

        Model(const Model&) = delete;
        Model& operator=(const Model&) = delete;

        Model(Model&& other) noexcept;
        Model& operator=(Model&& other) noexcept;

        // 기존 vector 버전(유지) - span 버전으로 위임
        bool Create(const std::vector<Vertex>& vertices,
            const std::vector<uint32_t>& indices,
            GLenum primitive = GL_TRIANGLES);

        // 추가: span 버전
        bool Create(std::span<const Vertex> vertices,
            std::span<const uint32_t> indices,
            GLenum primitive = GL_TRIANGLES);

        void Draw() const;

        GLenum Primitive() const;
    public:
        struct Bounds
        {
            glm::vec3 Min{ 0.0f };
            glm::vec3 Max{ 0.0f };

            glm::vec3 Center() const { return (Min + Max) * 0.5f; }
            glm::vec3 Extents() const { return (Max - Min) * 0.5f; }
        };

        const Bounds& GetBounds() const;
        float GetBoundingSphereRadius() const;
    private:
        void Destroy();

    private:
        GLuint m_vao{ 0 };
        GLuint m_vbo{ 0 };
        GLuint m_ebo{ 0 };

        Bounds m_bounds{};
        float m_boundRadius{ 0.0f };
        bool m_hasBounds{ false };

        GLsizei m_indexCount{ 0 };
        GLenum m_primitive{ GL_TRIANGLES };
    };
} // namespace gfx
