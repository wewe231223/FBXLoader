#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "ModelResult.h"
#include "SceneVisitor.h"

namespace asset {
    class MeshHierarchyBuilder final : public ISceneNodeVisitor {
    public:
        explicit MeshHierarchyBuilder(ModelResult& OutResult, const std::unordered_map<const ufbx_material*, std::size_t>* MaterialLookup);
        ~MeshHierarchyBuilder();

        MeshHierarchyBuilder(const MeshHierarchyBuilder& Other) = delete;
        MeshHierarchyBuilder& operator=(const MeshHierarchyBuilder& Other) = delete;
        MeshHierarchyBuilder(MeshHierarchyBuilder&& Other) = delete;
        MeshHierarchyBuilder& operator=(MeshHierarchyBuilder&& Other) = delete;

    public:
        void OnNodeBegin(const ufbx_scene& Scene, const ufbx_node& Node, const NodeVisitContext& Context) override;
        void OnNodeEnd(const ufbx_scene& Scene, const ufbx_node& Node) override;

    private:
        struct PackedVertex final {
        public:
            float Position[3]{ 0.0f, 0.0f, 0.0f };
            float Normal[3]{ 0.0f, 0.0f, 0.0f };
            float TexCoord[4][2]{ { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f } };
            float Color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
            float Tangent[3]{ 0.0f, 0.0f, 0.0f };
            float Bitangent[3]{ 0.0f, 0.0f, 0.0f };
            std::uint32_t BoneIndices[4]{ 0, 0, 0, 0 };
            float BoneWeights[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
        };

    private:
        static glm::vec3 ToGlmVec3(const ufbx_vec3& Value);
        static glm::vec2 ToGlmVec2(const ufbx_vec2& Value);
        static glm::vec4 ToGlmVec4(const ufbx_vec4& Value);

        PackedVertex MakePackedVertex(const ufbx_mesh& Mesh, std::uint32_t CornerIndex) const;

        glm::vec3 ReadPosition(const ufbx_mesh& Mesh, std::uint32_t Index) const;
        glm::vec3 ReadNormal(const ufbx_mesh& Mesh, std::uint32_t Index) const;
        glm::vec2 ReadTexCoord(const ufbx_mesh& Mesh, std::size_t SetIndex, std::uint32_t Index) const;
        glm::vec4 ReadColor(const ufbx_mesh& Mesh, std::uint32_t Index) const;
        glm::vec3 ReadTangent(const ufbx_mesh& Mesh, std::uint32_t Index) const;
        glm::vec3 ReadBitangent(const ufbx_mesh& Mesh, std::uint32_t Index) const;
        void ReadBoneData(const ufbx_mesh& Mesh, std::uint32_t CornerIndex, glm::uvec4& OutIndices, glm::vec4& OutWeights) const;

        void AppendIndexedMeshUfbx(const ufbx_mesh& Mesh, VertexAttributes& OutVertices, std::vector<std::uint32_t>& OutIndices) const;

    private:
        ModelResult& mResult;
        std::vector<ModelNode*> mNodeStack{};
        const std::unordered_map<const ufbx_material*, std::size_t>* mMaterialLookup{ nullptr };
    };
}
