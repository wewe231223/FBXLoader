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


    enum class MaterialType : std::uint16_t {
        DiffuseFactor,
        DiffuseFactorMap,
        DiffuseColor,
        DiffuseColorMap,
        SpecularFactor,
        SpecularFactorMap,
        SpecularColor,
        SpecularColorMap,
        SpecularExponent,
        SpecularExponentMap,
        ReflectionFactor,
        ReflectionFactorMap,
        ReflectionColor,
        ReflectionColorMap,
        TransparencyFactor,
        TransparencyFactorMap,
        TransparencyColor,
        TransparencyColorMap,
        EmissionFactor,
        EmissionFactorMap,
        EmissionColor,
        EmissionColorMap,
        AmbientFactor,
        AmbientFactorMap,
        AmbientColor,
        AmbientColorMap,
        NormalMap,
        NormalMapMap,
        Bump,
        BumpMap,
        BumpFactor,
        BumpFactorMap,
        DisplacementFactor,
        DisplacementFactorMap,
        Displacement,
        DisplacementMap,
        VectorDisplacementFactor,
        VectorDisplacementFactorMap,
        VectorDisplacement,
        VectorDisplacementMap,
        BaseFactor,
        BaseFactorMap,
        BaseColor,
        BaseColorMap,
        Roughness,
        RoughnessMap,
        Metalness,
        MetalnessMap,
        DiffuseRoughness,
        DiffuseRoughnessMap,
        SpecularFactorPbr,
        SpecularFactorPbrMap,
        SpecularColorPbr,
        SpecularColorPbrMap,
        SpecularIor,
        SpecularIorMap,
        SpecularAnisotropy,
        SpecularAnisotropyMap,
        SpecularRotation,
        SpecularRotationMap,
        TransmissionFactor,
        TransmissionFactorMap,
        TransmissionColor,
        TransmissionColorMap,
        TransmissionDepth,
        TransmissionDepthMap,
        TransmissionScatter,
        TransmissionScatterMap,
        TransmissionScatterAnisotropy,
        TransmissionScatterAnisotropyMap,
        TransmissionDispersion,
        TransmissionDispersionMap,
        TransmissionRoughness,
        TransmissionRoughnessMap,
        TransmissionExtraRoughness,
        TransmissionExtraRoughnessMap,
        TransmissionPriority,
        TransmissionPriorityMap,
        TransmissionEnableInAov,
        TransmissionEnableInAovMap,
        SubsurfaceFactor,
        SubsurfaceFactorMap,
        SubsurfaceColor,
        SubsurfaceColorMap,
        SubsurfaceRadius,
        SubsurfaceRadiusMap,
        SubsurfaceScale,
        SubsurfaceScaleMap,
        SubsurfaceAnisotropy,
        SubsurfaceAnisotropyMap,
        SubsurfaceTintColor,
        SubsurfaceTintColorMap,
        SubsurfaceType,
        SubsurfaceTypeMap,
        SheenFactor,
        SheenFactorMap,
        SheenColor,
        SheenColorMap,
        SheenRoughness,
        SheenRoughnessMap,
        CoatFactor,
        CoatFactorMap,
        CoatColor,
        CoatColorMap,
        CoatRoughness,
        CoatRoughnessMap,
        CoatIor,
        CoatIorMap,
        CoatAnisotropy,
        CoatAnisotropyMap,
        CoatRotation,
        CoatRotationMap,
        CoatNormal,
        CoatNormalMap,
        CoatAffectBaseColor,
        CoatAffectBaseColorMap,
        CoatAffectBaseRoughness,
        CoatAffectBaseRoughnessMap,
        ThinFilmFactor,
        ThinFilmFactorMap,
        ThinFilmThickness,
        ThinFilmThicknessMap,
        ThinFilmIor,
        ThinFilmIorMap,
        EmissionFactorPbr,
        EmissionFactorPbrMap,
        EmissionColorPbr,
        EmissionColorPbrMap,
        Opacity,
        OpacityMap,
        IndirectDiffuse,
        IndirectDiffuseMap,
        IndirectSpecular,
        IndirectSpecularMap,
        NormalMapPbr,
        NormalMapPbrMap,
        TangentMap,
        TangentMapMap,
        DisplacementMapPbr,
        DisplacementMapPbrMap,
        MatteFactor,
        MatteFactorMap,
        MatteColor,
        MatteColorMap,
        AmbientOcclusion,
        AmbientOcclusionMap,
        Glossiness,
        GlossinessMap,
        CoatGlossiness,
        CoatGlossinessMap,
        TransmissionGlossiness,
        TransmissionGlossinessMap,
    };

    enum class MaterialMapKind : std::uint8_t {
        None,
        Real,
        Vec2,
        Vec3,
        Vec4,
        Int,
        Bool,
        String,
    };

    struct MaterialMap final {
    public:
        MaterialMap();
        explicit MaterialMap(float Value);
        explicit MaterialMap(std::int64_t Value);
        explicit MaterialMap(bool Value);
        explicit MaterialMap(const glm::vec2& Value);
        explicit MaterialMap(const glm::vec3& Value);
        explicit MaterialMap(const glm::vec4& Value);
        explicit MaterialMap(std::string Value);

        MaterialMapKind GetKind() const;
        float GetReal() const;
        std::int64_t GetInt() const;
        bool GetBool() const;
        glm::vec2 GetVec2() const;
        glm::vec3 GetVec3() const;
        glm::vec4 GetVec4() const;
        const std::string& GetString() const;

    private:
        MaterialMapKind mKind{ MaterialMapKind::None };
        union {
            float mReal;
            std::int64_t mInt;
            bool mBool;
            float mVec2[2];
            float mVec3[3];
            float mVec4[4];
        } mValue;
        std::string mString{};
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
