#include "Model.h"

#include <cstddef>

namespace asset
{
    Model::~Model()
    {
        Destroy();
    }

    Model::Model(Model&& other) noexcept
        : m_vao(other.m_vao)
        , m_vbo(other.m_vbo)
        , m_ebo(other.m_ebo)
        , m_indexCount(other.m_indexCount)
        , m_primitive(other.m_primitive)
    {
        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_ebo = 0;
        other.m_indexCount = 0;
        other.m_primitive = GL_TRIANGLES;
    }

    Model& Model::operator=(Model&& other) noexcept
    {
        if (this != &other)
        {
            Destroy();

            m_vao = other.m_vao;
            m_vbo = other.m_vbo;
            m_ebo = other.m_ebo;
            m_indexCount = other.m_indexCount;
            m_primitive = other.m_primitive;

            other.m_vao = 0;
            other.m_vbo = 0;
            other.m_ebo = 0;
            other.m_indexCount = 0;
            other.m_primitive = GL_TRIANGLES;
        }
        return *this;
    }

    void Model::Destroy()
    {
        if (m_ebo != 0)
        {
            glDeleteBuffers(1, &m_ebo);
            m_ebo = 0;
        }
        if (m_vbo != 0)
        {
            glDeleteBuffers(1, &m_vbo);
            m_vbo = 0;
        }
        if (m_vao != 0)
        {
            glDeleteVertexArrays(1, &m_vao);
            m_vao = 0;
        }

        m_indexCount = 0;
    }

    bool Model::Create(const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        GLenum primitive)
    {
        return Create(std::span<const Vertex>(vertices),
            std::span<const uint32_t>(indices),
            primitive);
    }

    bool Model::Create(std::span<const Vertex> vertices,
        std::span<const uint32_t> indices,
        GLenum primitive)
    {
        Destroy();

        if (vertices.empty() || indices.empty())
        {
            return false;
        }

        m_bounds.Min = vertices[0].Position;
        m_bounds.Max = vertices[0].Position;

        for (const auto& vx : vertices)
        {
            m_bounds.Min = glm::min(m_bounds.Min, vx.Position);
            m_bounds.Max = glm::max(m_bounds.Max, vx.Position);
        }

        const glm::vec3 ext = m_bounds.Extents();
        m_boundRadius = glm::length(ext);
        m_hasBounds = true;

        m_indexCount = static_cast<GLsizei>(indices.size());
        m_primitive = primitive;

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ebo);

        glBindVertexArray(m_vao);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices.size_bytes()),
            vertices.data(),
            GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(indices.size_bytes()),
            indices.data(),
            GL_STATIC_DRAW);

        // layout(location = 0) vec3 aPos;
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, Position)));

        // layout(location = 1) vec3 aNormal;
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, Normal)));

        // layout(location = 2) vec2 aUV;
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, TexCoord)));

        // layout(location = 3) vec4 aColor;
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, Color)));

        glBindVertexArray(0);

        return true;
    }

    void Model::Draw() const
    {
        glBindVertexArray(m_vao);
        glDrawElements(m_primitive, m_indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    GLenum Model::Primitive() const
    {
        return m_primitive;
    }

    const Model::Bounds& Model::GetBounds() const
    {
        return m_bounds;
    }

    float Model::GetBoundingSphereRadius() const
    {
        return m_boundRadius;
    }
} // namespace gfx
