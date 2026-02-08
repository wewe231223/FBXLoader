#pragma once

#include <cstdint>
#include <vector>

#include "ModelResult.h"
#include "SceneVisitor.h"

namespace asset {
    class MeshHierarchyBuilder final : public ISceneNodeVisitor {
    public:
        MeshHierarchyBuilder(ModelResult& OutResult);
        ~MeshHierarchyBuilder();

        MeshHierarchyBuilder(const MeshHierarchyBuilder& Other) = delete;
        MeshHierarchyBuilder& operator=(const MeshHierarchyBuilder& Other) = delete;
        MeshHierarchyBuilder(MeshHierarchyBuilder&& Other) = delete;
        MeshHierarchyBuilder& operator=(MeshHierarchyBuilder&& Other) = delete;

    public:
        void OnNodeBegin(const ufbx_scene& Scene, const ufbx_node& Node, const NodeVisitContext& Ctx) override;
        void OnNodeEnd(const ufbx_scene& Scene, const ufbx_node& Node) override;

    private:
        struct PackedVertex final {
            float Position[3]{ 0.0f, 0.0f, 0.0f };
            float Normal[3]{ 0.0f, 0.0f, 0.0f };
            float TexCoord[2]{ 0.0f, 0.0f };
            float Color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
        };

    private:
        static glm::vec3 ToGlmVec3(const ufbx_vec3& V);
        static glm::vec2 ToGlmVec2(const ufbx_vec2& V);
        static glm::vec4 ToGlmVec4(const ufbx_vec4& V);

        PackedVertex MakePackedVertex(const ufbx_mesh& Mesh, std::uint32_t CornerIndex) const;

        glm::vec3 ReadPosition(const ufbx_mesh& Mesh, std::uint32_t Index) const;
        glm::vec3 ReadNormal(const ufbx_mesh& Mesh, std::uint32_t Index) const;
        glm::vec2 ReadTexCoord(const ufbx_mesh& Mesh, std::uint32_t Index) const;
        glm::vec4 ReadColor(const ufbx_mesh& Mesh, std::uint32_t Index) const;

        void AppendIndexedMeshUfbx(const ufbx_mesh& Mesh, DataList& OutVertices, DataList& OutIndices) const;

    private:
        ModelResult& mResult;
        std::vector<ModelNode*> mNodeStack{};
    };
}