#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <stdexcept>
#include <utility>
#include <type_traits>
#include <new>
#include <cassert>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ufbx.h"

namespace asset {
    enum class GraphicsAPI : std::uint8_t {
        DirectX,
        OpenGL,
    };

    struct Vertex final {
    public:
        glm::vec3 Position{ 0.0f, 0.0f, 0.0f };
        glm::vec3 Normal{ 0.0f, 0.0f, 0.0f };
        glm::vec2 TexCoord{ 0.0f, 0.0f };
        glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
    };

    struct AssetError final : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };
}