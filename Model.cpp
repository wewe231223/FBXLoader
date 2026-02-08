#include "Model.h"

#include <cstddef>

namespace asset {
    Model::~Model() {
        Destroy();
    }

    Model::Model(Model&& Other) noexcept
        : mVao{ Other.mVao },
		mIndexBuffer{ Other.mIndexBuffer },
		mPositionBuffer{ Other.mPositionBuffer },
		mNormalBuffer{ Other.mNormalBuffer },
		mTexCoordBuffers{ Other.mTexCoordBuffers },
		mColorBuffer{ Other.mColorBuffer },
		mTangentBuffer{ Other.mTangentBuffer },
		mBitangentBuffer{ Other.mBitangentBuffer },
		mBoneIndexBuffer{ Other.mBoneIndexBuffer },
		mBoneWeightBuffer{ Other.mBoneWeightBuffer },
		mBounds{ Other.mBounds },
		mBoundRadius{ Other.mBoundRadius },
		mHasBounds{ Other.mHasBounds },
		mIndexCount{ Other.mIndexCount },
		mPrimitive{ Other.mPrimitive } {
        Other.mVao = 0;
        Other.mIndexBuffer = 0;
        Other.mPositionBuffer = 0;
        Other.mNormalBuffer = 0;
        Other.mTexCoordBuffers = {};
        Other.mColorBuffer = 0;
        Other.mTangentBuffer = 0;
        Other.mBitangentBuffer = 0;
        Other.mBoneIndexBuffer = 0;
        Other.mBoneWeightBuffer = 0;
        Other.mBounds = {};
        Other.mBoundRadius = 0.0f;
        Other.mHasBounds = false;
        Other.mIndexCount = 0;
        Other.mPrimitive = GL_TRIANGLES;
    }

    Model& Model::operator=(Model&& Other) noexcept {
        if (this != &Other) {
            Destroy();

            mVao = Other.mVao;
            mIndexBuffer = Other.mIndexBuffer;
            mPositionBuffer = Other.mPositionBuffer;
            mNormalBuffer = Other.mNormalBuffer;
            mTexCoordBuffers = Other.mTexCoordBuffers;
            mColorBuffer = Other.mColorBuffer;
            mTangentBuffer = Other.mTangentBuffer;
            mBitangentBuffer = Other.mBitangentBuffer;
            mBoneIndexBuffer = Other.mBoneIndexBuffer;
            mBoneWeightBuffer = Other.mBoneWeightBuffer;
            mBounds = Other.mBounds;
            mBoundRadius = Other.mBoundRadius;
            mHasBounds = Other.mHasBounds;
            mIndexCount = Other.mIndexCount;
            mPrimitive = Other.mPrimitive;

            Other.mVao = 0;
            Other.mIndexBuffer = 0;
            Other.mPositionBuffer = 0;
            Other.mNormalBuffer = 0;
            Other.mTexCoordBuffers = {};
            Other.mColorBuffer = 0;
            Other.mTangentBuffer = 0;
            Other.mBitangentBuffer = 0;
            Other.mBoneIndexBuffer = 0;
            Other.mBoneWeightBuffer = 0;
            Other.mBounds = {};
            Other.mBoundRadius = 0.0f;
            Other.mHasBounds = false;
            Other.mIndexCount = 0;
            Other.mPrimitive = GL_TRIANGLES;
        }
        return *this;
    }

    glm::vec3 Model::Bounds::Center() const {
        return (Min + Max) * 0.5f;
    }

    glm::vec3 Model::Bounds::Extents() const {
        return (Max - Min) * 0.5f;
    }

    bool Model::Create(const VertexAttributes& Vertices, const std::vector<std::uint32_t>& Indices, GLenum Primitive) {
        return Create(Vertices, std::span<const std::uint32_t>{ Indices.data(), Indices.size() }, Primitive);
    }

    bool Model::Create(const VertexAttributes& Vertices, std::span<const std::uint32_t> Indices, GLenum Primitive) {
        Destroy();

        if (!ValidateVertexData(Vertices) || Indices.empty()) {
            return false;
        }

        mBounds.Min = Vertices.Positions.front();
        mBounds.Max = Vertices.Positions.front();

        for (const glm::vec3& Position : Vertices.Positions) {
            mBounds.Min = glm::min(mBounds.Min, Position);
            mBounds.Max = glm::max(mBounds.Max, Position);
        }

        const glm::vec3 Extents{ mBounds.Extents() };
        mBoundRadius = glm::length(Extents);
        mHasBounds = true;

        mIndexCount = static_cast<GLsizei>(Indices.size());
        mPrimitive = Primitive;

        glGenVertexArrays(1, &mVao);
        glGenBuffers(1, &mIndexBuffer);

        glBindVertexArray(mVao);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(Indices.size_bytes()), Indices.data(), GL_STATIC_DRAW);

        SetupVertexBuffer(mPositionBuffer, std::span<const std::byte>{ reinterpret_cast<const std::byte*>(Vertices.Positions.data()), Vertices.Positions.size() * sizeof(glm::vec3) }, 0, 3, GL_FLOAT, false, false);
        SetupVertexBuffer(mNormalBuffer, std::span<const std::byte>{ reinterpret_cast<const std::byte*>(Vertices.Normals.data()), Vertices.Normals.size() * sizeof(glm::vec3) }, 1, 3, GL_FLOAT, false, false);

        for (std::size_t Index{ 0 }; Index < Vertices.TexCoords.size(); ++Index) {
            const std::vector<glm::vec2>& TexCoords{ Vertices.TexCoords[Index] };
            const GLuint AttributeIndex{ static_cast<GLuint>(2 + Index) };
            SetupVertexBuffer(mTexCoordBuffers[Index], std::span<const std::byte>{ reinterpret_cast<const std::byte*>(TexCoords.data()), TexCoords.size() * sizeof(glm::vec2) }, AttributeIndex, 2, GL_FLOAT, false, false);
        }

        SetupVertexBuffer(mColorBuffer, std::span<const std::byte>{ reinterpret_cast<const std::byte*>(Vertices.Colors.data()), Vertices.Colors.size() * sizeof(glm::vec4) }, 6, 4, GL_FLOAT, false, false);
        SetupVertexBuffer(mTangentBuffer, std::span<const std::byte>{ reinterpret_cast<const std::byte*>(Vertices.Tangents.data()), Vertices.Tangents.size() * sizeof(glm::vec3) }, 7, 3, GL_FLOAT, false, false);
        SetupVertexBuffer(mBitangentBuffer, std::span<const std::byte>{ reinterpret_cast<const std::byte*>(Vertices.Bitangents.data()), Vertices.Bitangents.size() * sizeof(glm::vec3) }, 8, 3, GL_FLOAT, false, false);
        SetupVertexBuffer(mBoneIndexBuffer, std::span<const std::byte>{ reinterpret_cast<const std::byte*>(Vertices.BoneIndices.data()), Vertices.BoneIndices.size() * sizeof(glm::uvec4) }, 9, 4, GL_UNSIGNED_INT, false, true);
        SetupVertexBuffer(mBoneWeightBuffer, std::span<const std::byte>{ reinterpret_cast<const std::byte*>(Vertices.BoneWeights.data()), Vertices.BoneWeights.size() * sizeof(glm::vec4) }, 10, 4, GL_FLOAT, false, false);

        glBindVertexArray(0);

        return true;
    }

    void Model::Draw() const {
        glBindVertexArray(mVao);
        glDrawElements(mPrimitive, mIndexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    GLenum Model::Primitive() const {
        return mPrimitive;
    }

    const Model::Bounds& Model::GetBounds() const {
        return mBounds;
    }

    float Model::GetBoundingSphereRadius() const {
        return mBoundRadius;
    }

    void Model::Destroy() {
        if (mIndexBuffer != 0) {
            glDeleteBuffers(1, &mIndexBuffer);
            mIndexBuffer = 0;
        }
        if (mPositionBuffer != 0) {
            glDeleteBuffers(1, &mPositionBuffer);
            mPositionBuffer = 0;
        }
        if (mNormalBuffer != 0) {
            glDeleteBuffers(1, &mNormalBuffer);
            mNormalBuffer = 0;
        }
        for (GLuint& Buffer : mTexCoordBuffers) {
            if (Buffer != 0) {
                glDeleteBuffers(1, &Buffer);
                Buffer = 0;
            }
        }
        if (mColorBuffer != 0) {
            glDeleteBuffers(1, &mColorBuffer);
            mColorBuffer = 0;
        }
        if (mTangentBuffer != 0) {
            glDeleteBuffers(1, &mTangentBuffer);
            mTangentBuffer = 0;
        }
        if (mBitangentBuffer != 0) {
            glDeleteBuffers(1, &mBitangentBuffer);
            mBitangentBuffer = 0;
        }
        if (mBoneIndexBuffer != 0) {
            glDeleteBuffers(1, &mBoneIndexBuffer);
            mBoneIndexBuffer = 0;
        }
        if (mBoneWeightBuffer != 0) {
            glDeleteBuffers(1, &mBoneWeightBuffer);
            mBoneWeightBuffer = 0;
        }
        if (mVao != 0) {
            glDeleteVertexArrays(1, &mVao);
            mVao = 0;
        }

        mIndexCount = 0;
    }

    bool Model::ValidateVertexData(const VertexAttributes& Vertices) const {
        const std::size_t Count{ Vertices.Positions.size() };
        if (Count == 0) {
            return false;
        }
        if (!Vertices.Normals.empty() && Vertices.Normals.size() != Count) {
            return false;
        }
        for (const std::vector<glm::vec2>& TexCoords : Vertices.TexCoords) {
            if (!TexCoords.empty() && TexCoords.size() != Count) {
                return false;
            }
        }
        if (!Vertices.Colors.empty() && Vertices.Colors.size() != Count) {
            return false;
        }
        if (!Vertices.Tangents.empty() && Vertices.Tangents.size() != Count) {
            return false;
        }
        if (!Vertices.Bitangents.empty() && Vertices.Bitangents.size() != Count) {
            return false;
        }
        if (!Vertices.BoneIndices.empty() && Vertices.BoneIndices.size() != Count) {
            return false;
        }
        if (!Vertices.BoneWeights.empty() && Vertices.BoneWeights.size() != Count) {
            return false;
        }
        return true;
    }

    void Model::SetupVertexBuffer(GLuint& Buffer, std::span<const std::byte> Bytes, GLuint AttributeIndex, GLint ComponentCount, GLenum Type, bool Normalized, bool IsInteger) {
        if (Bytes.empty()) {
            return;
        }
        glGenBuffers(1, &Buffer);
        glBindBuffer(GL_ARRAY_BUFFER, Buffer);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(Bytes.size()), Bytes.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(AttributeIndex);
        if (IsInteger) {
            glVertexAttribIPointer(AttributeIndex, ComponentCount, Type, 0, nullptr);
            return;
        }
        glVertexAttribPointer(AttributeIndex, ComponentCount, Type, Normalized ? GL_TRUE : GL_FALSE, 0, nullptr);
    }
}
