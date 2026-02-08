#pragma once

#include <array>
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

    struct VertexAttributes final {
    public:
        std::vector<glm::vec3> Positions{};
        std::vector<glm::vec3> Normals{};
        std::array<std::vector<glm::vec2>, 4> TexCoords{};
        std::vector<glm::vec4> Colors{};
        std::vector<glm::vec3> Tangents{};
        std::vector<glm::vec3> Bitangents{};
        std::vector<glm::uvec4> BoneIndices{};
        std::vector<glm::vec4> BoneWeights{};

        std::size_t VertexCount() const;
        bool Empty() const;

        void Reserve(std::size_t Count);
        void Resize(std::size_t Count);
    };


	enum MaterialType : std::uint8_t {
		DiffuseColor, 
        DiffuseTexture, 
        // TODO ... 
	};

    struct MaterialMap {
        // TODO ... 
    };

    struct MaterialProperty {
		MaterialType Type;
        MaterialMap Data; 
    };

    struct Material {
		std::vector<MaterialProperty> Properties;
    };


    struct AssetError final : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };
}
