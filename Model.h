#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include <glad/glad.h>

#include "Common.h"

namespace asset {
    class Model {
    public:
        struct Bounds {
        public:
            glm::vec3 Min{ 0.0f };
            glm::vec3 Max{ 0.0f };

            glm::vec3 Center() const;
            glm::vec3 Extents() const;
        };

    public:
        Model() = default;
        ~Model();

        Model(const Model& Other) = delete;
        Model& operator=(const Model& Other) = delete;

        Model(Model&& Other) noexcept;
        Model& operator=(Model&& Other) noexcept;

    public:
        bool Create(const VertexAttributes& Vertices, const std::vector<std::uint32_t>& Indices, GLenum Primitive = GL_TRIANGLES);
        bool Create(const VertexAttributes& Vertices, std::span<const std::uint32_t> Indices, GLenum Primitive = GL_TRIANGLES);

        void Draw() const;

        GLenum Primitive() const;

        const Bounds& GetBounds() const;
        float GetBoundingSphereRadius() const;

    private:
        void Destroy();
        bool ValidateVertexData(const VertexAttributes& Vertices) const;
        void SetupVertexBuffer(GLuint& Buffer, std::span<const std::byte> Bytes, GLuint AttributeIndex, GLint ComponentCount, GLenum Type, bool Normalized, bool IsInteger);

    private:
        GLuint mVao{ 0 };
        GLuint mIndexBuffer{ 0 };
        GLuint mPositionBuffer{ 0 };
        GLuint mNormalBuffer{ 0 };
        std::array<GLuint, 4> mTexCoordBuffers{};
        GLuint mColorBuffer{ 0 };
        GLuint mTangentBuffer{ 0 };
        GLuint mBitangentBuffer{ 0 };
        GLuint mBoneIndexBuffer{ 0 };
        GLuint mBoneWeightBuffer{ 0 };

        Bounds mBounds{};
        float mBoundRadius{ 0.0f };
        bool mHasBounds{ false };

        GLsizei mIndexCount{ 0 };
        GLenum mPrimitive{ GL_TRIANGLES };
    };
}
