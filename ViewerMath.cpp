#include "ViewerMath.h"

#include <cstddef>

namespace asset {
    glm::vec2 ToGlmVec2(const Vec2& Value) {
        return glm::vec2{ Value.mX, Value.mY };
    }

    glm::vec3 ToGlmVec3(const Vec3& Value) {
        return glm::vec3{ Value.mX, Value.mY, Value.mZ };
    }

    glm::vec4 ToGlmVec4(const Vec4& Value) {
        return glm::vec4{ Value.mX, Value.mY, Value.mZ, Value.mW };
    }

    glm::uvec4 ToGlmUVec4(const UVec4& Value) {
        return glm::uvec4{ Value.mX, Value.mY, Value.mZ, Value.mW };
    }

    glm::mat4 ToGlmMat4(const Mat4& Value) {
        glm::mat4 Result{ 1.0f };
        for (std::size_t Row{ 0 }; Row < 4; ++Row) {
            for (std::size_t Col{ 0 }; Col < 4; ++Col) {
                Result[Col][Row] = Value.mValue[Row][Col];
            }
        }
        return Result;
    }

    Vec2 ToAssetVec2(const glm::vec2& Value) {
        return Vec2{ Value.x, Value.y };
    }

    Vec3 ToAssetVec3(const glm::vec3& Value) {
        return Vec3{ Value.x, Value.y, Value.z };
    }

    Vec4 ToAssetVec4(const glm::vec4& Value) {
        return Vec4{ Value.x, Value.y, Value.z, Value.w };
    }

    UVec4 ToAssetUVec4(const glm::uvec4& Value) {
        return UVec4{ Value.x, Value.y, Value.z, Value.w };
    }
}
