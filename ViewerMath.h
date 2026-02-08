#pragma once

#include <glm/glm.hpp>

#include "NumericTypes.h"

namespace asset {
    glm::vec2 ToGlmVec2(const Vec2& Value);
    glm::vec3 ToGlmVec3(const Vec3& Value);
    glm::vec4 ToGlmVec4(const Vec4& Value);
    glm::uvec4 ToGlmUVec4(const UVec4& Value);
    glm::mat4 ToGlmMat4(const Mat4& Value);

    Vec2 ToAssetVec2(const glm::vec2& Value);
    Vec3 ToAssetVec3(const glm::vec3& Value);
    Vec4 ToAssetVec4(const glm::vec4& Value);
    UVec4 ToAssetUVec4(const glm::uvec4& Value);
}
