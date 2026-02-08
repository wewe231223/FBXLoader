#include "MeshHierarchyBuilder.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

using namespace asset;

MeshHierarchyBuilder::MeshHierarchyBuilder(ModelResult& OutResult, const std::unordered_map<const ufbx_material*, std::size_t>* MaterialLookup)
    : mResult{ OutResult }
    , mMaterialLookup{ MaterialLookup } {
}

MeshHierarchyBuilder::~MeshHierarchyBuilder() = default;

void MeshHierarchyBuilder::OnNodeBegin(const ufbx_scene& Scene, const ufbx_node& Node, const NodeVisitContext& Context) {
    static_cast<void>(Scene);
    ModelNode* ParentNode{ nullptr };
    if (!mNodeStack.empty()) {
        ParentNode = mNodeStack.back();
    }

    const std::string Name{ (Node.name.data != nullptr) ? std::string{ Node.name.data, Node.name.length } : std::string{ "Unnamed" } };

    ModelNode& OutNode{ mResult.CreateNode(Name, ParentNode) };
    OutNode.SetNodeToParent(Context.mNodeToParent);
    OutNode.SetGeometryToNode(Context.mGeometryToNode);
    if (mMaterialLookup != nullptr && Node.materials.count > 0) {
        std::vector<std::size_t> MaterialIndices{};
        MaterialIndices.reserve(Node.materials.count);
        for (std::size_t Index{ 0 }; Index < Node.materials.count; ++Index) {
            const ufbx_material* MaterialData{ Node.materials.data[Index] };
            if (MaterialData == nullptr) {
                continue;
            }
            auto Lookup{ mMaterialLookup->find(MaterialData) };
            if (Lookup == mMaterialLookup->end()) {
                continue;
            }
            MaterialIndices.push_back(Lookup->second);
        }
        if (!MaterialIndices.empty()) {
            OutNode.SetMaterialIndices(std::move(MaterialIndices));
        }
    }
    if (Node.mesh != nullptr) {
        AppendIndexedMeshUfbx(*Node.mesh, OutNode.Vertices(), OutNode.Indices());
    }
    mNodeStack.push_back(&OutNode);
}

void MeshHierarchyBuilder::OnNodeEnd(const ufbx_scene& Scene, const ufbx_node& Node) {
    static_cast<void>(Scene);
    static_cast<void>(Node);
    if (!mNodeStack.empty()) {
        mNodeStack.pop_back();
        return;
    }
    throw AssetError{ "MeshHierarchyBuilder: node stack underflow" };
}

glm::vec3 MeshHierarchyBuilder::ToGlmVec3(const ufbx_vec3& Value) {
    return glm::vec3{ static_cast<float>(Value.x), static_cast<float>(Value.y), static_cast<float>(Value.z) };
}

glm::vec2 MeshHierarchyBuilder::ToGlmVec2(const ufbx_vec2& Value) {
    return glm::vec2{ static_cast<float>(Value.x), static_cast<float>(Value.y) };
}

glm::vec4 MeshHierarchyBuilder::ToGlmVec4(const ufbx_vec4& Value) {
    return glm::vec4{ static_cast<float>(Value.x), static_cast<float>(Value.y), static_cast<float>(Value.z), static_cast<float>(Value.w) };
}

glm::vec3 MeshHierarchyBuilder::ReadPosition(const ufbx_mesh& Mesh, std::uint32_t Index) const {
    const ufbx_vec3 Value{ ufbx_get_vertex_vec3(&Mesh.vertex_position, Index) };
    return ToGlmVec3(Value);
}

glm::vec3 MeshHierarchyBuilder::ReadNormal(const ufbx_mesh& Mesh, std::uint32_t Index) const {
    if (Mesh.vertex_normal.exists) {
        const ufbx_vec3 Value{ ufbx_get_vertex_vec3(&Mesh.vertex_normal, Index) };
        return glm::normalize(ToGlmVec3(Value));
    }
    return glm::vec3{ 0.0f, 1.0f, 0.0f };
}

glm::vec2 MeshHierarchyBuilder::ReadTexCoord(const ufbx_mesh& Mesh, std::size_t SetIndex, std::uint32_t Index) const {
    if (SetIndex == 0) {
        if (Mesh.vertex_uv.exists) {
            const ufbx_vec2 Value{ ufbx_get_vertex_vec2(&Mesh.vertex_uv, Index) };
            return ToGlmVec2(Value);
        }
        return glm::vec2{ 0.0f, 0.0f };
    }

    if (SetIndex < Mesh.uv_sets.count) {
        const ufbx_uv_set& Set{ Mesh.uv_sets.data[SetIndex] };
        if (Set.vertex_uv.exists) {
            const ufbx_vec2 Value{ ufbx_get_vertex_vec2(&Set.vertex_uv, Index) };
            return ToGlmVec2(Value);
        }
    }
    return glm::vec2{ 0.0f, 0.0f };
}

glm::vec4 MeshHierarchyBuilder::ReadColor(const ufbx_mesh& Mesh, std::uint32_t Index) const {
    if (Mesh.vertex_color.exists) {
        const ufbx_vec4 Value{ ufbx_get_vertex_vec4(&Mesh.vertex_color, Index) };
        return ToGlmVec4(Value);
    }
    return glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
}

glm::vec3 MeshHierarchyBuilder::ReadTangent(const ufbx_mesh& Mesh, std::uint32_t Index) const {
    if (Mesh.vertex_tangent.exists) {
        const ufbx_vec3 Value{ ufbx_get_vertex_vec3(&Mesh.vertex_tangent, Index) };
        return ToGlmVec3(Value);
    }
    return glm::vec3{ 0.0f, 0.0f, 0.0f };
}

glm::vec3 MeshHierarchyBuilder::ReadBitangent(const ufbx_mesh& Mesh, std::uint32_t Index) const {
    if (Mesh.vertex_bitangent.exists) {
        const ufbx_vec3 Value{ ufbx_get_vertex_vec3(&Mesh.vertex_bitangent, Index) };
        return ToGlmVec3(Value);
    }
    return glm::vec3{ 0.0f, 0.0f, 0.0f };
}

void MeshHierarchyBuilder::ReadBoneData(const ufbx_mesh& Mesh, std::uint32_t CornerIndex, glm::uvec4& OutIndices, glm::vec4& OutWeights) const {
    OutIndices = glm::uvec4{ 0, 0, 0, 0 };
    OutWeights = glm::vec4{ 0.0f, 0.0f, 0.0f, 0.0f };

    if (Mesh.skin_deformers.count == 0) {
        return;
    }

    const ufbx_skin_deformer* Skin{ Mesh.skin_deformers.data[0] };
    if (Skin == nullptr) {
        return;
    }

    if (CornerIndex >= Mesh.vertex_indices.count) {
        return;
    }

    const std::uint32_t VertexIndex{ Mesh.vertex_indices.data[CornerIndex] };
    if (VertexIndex >= Skin->vertices.count) {
        return;
    }

    const ufbx_skin_vertex& SkinVertex{ Skin->vertices.data[VertexIndex] };
    const std::size_t WeightCount{ std::min<std::size_t>(SkinVertex.num_weights, 4) };
    if (WeightCount == 0) {
        return;
    }

    float WeightSum{ 0.0f };
    for (std::size_t Index{ 0 }; Index < WeightCount; ++Index) {
        const std::size_t WeightIndex{ SkinVertex.weight_begin + Index };
        if (WeightIndex >= Skin->weights.count) {
            break;
        }
        const ufbx_skin_weight& Weight{ Skin->weights.data[WeightIndex] };
        OutIndices[static_cast<int>(Index)] = Weight.cluster_index;
        OutWeights[static_cast<int>(Index)] = static_cast<float>(Weight.weight);
        WeightSum += OutWeights[static_cast<int>(Index)];

        
    }

    if (WeightSum > 0.0f) {
        OutWeights /= WeightSum;
    }
}

MeshHierarchyBuilder::PackedVertex MeshHierarchyBuilder::MakePackedVertex(const ufbx_mesh& Mesh, std::uint32_t CornerIndex) const {
    PackedVertex Packed{};
    const glm::vec3 Position{ ReadPosition(Mesh, CornerIndex) };
    const glm::vec3 Normal{ ReadNormal(Mesh, CornerIndex) };
    const glm::vec4 Color{ ReadColor(Mesh, CornerIndex) };
    const glm::vec3 Tangent{ ReadTangent(Mesh, CornerIndex) };
    const glm::vec3 Bitangent{ ReadBitangent(Mesh, CornerIndex) };
    glm::uvec4 BoneIndices{};
    glm::vec4 BoneWeights{};
    ReadBoneData(Mesh, CornerIndex, BoneIndices, BoneWeights);

    Packed.Position[0] = Position.x;
    Packed.Position[1] = Position.y;
    Packed.Position[2] = Position.z;
    Packed.Normal[0] = Normal.x;
    Packed.Normal[1] = Normal.y;
    Packed.Normal[2] = Normal.z;
    Packed.Color[0] = Color.x;
    Packed.Color[1] = Color.y;
    Packed.Color[2] = Color.z;
    Packed.Color[3] = Color.w;
    Packed.Tangent[0] = Tangent.x;
    Packed.Tangent[1] = Tangent.y;
    Packed.Tangent[2] = Tangent.z;
    Packed.Bitangent[0] = Bitangent.x;
    Packed.Bitangent[1] = Bitangent.y;
    Packed.Bitangent[2] = Bitangent.z;
    Packed.BoneIndices[0] = BoneIndices.x;
    Packed.BoneIndices[1] = BoneIndices.y;
    Packed.BoneIndices[2] = BoneIndices.z;
    Packed.BoneIndices[3] = BoneIndices.w;
    Packed.BoneWeights[0] = BoneWeights.x;
    Packed.BoneWeights[1] = BoneWeights.y;
    Packed.BoneWeights[2] = BoneWeights.z;
    Packed.BoneWeights[3] = BoneWeights.w;

    for (std::size_t SetIndex{ 0 }; SetIndex < 4; ++SetIndex) {
        const glm::vec2 TexCoord{ ReadTexCoord(Mesh, SetIndex, CornerIndex) };
        Packed.TexCoord[SetIndex][0] = TexCoord.x;
        Packed.TexCoord[SetIndex][1] = TexCoord.y;
    }

    return Packed;
}

void MeshHierarchyBuilder::AppendIndexedMeshUfbx(const ufbx_mesh& Mesh, VertexAttributes& OutVertices, std::vector<std::uint32_t>& OutIndices) const {
    const std::size_t NumCorners{ Mesh.num_indices };
    if (NumCorners == 0) {
        return;
    }

    std::vector<PackedVertex> CornerVertices{};
    CornerVertices.resize(NumCorners);
    for (std::size_t CornerIndex{ 0 }; CornerIndex < NumCorners; ++CornerIndex) {
        CornerVertices[CornerIndex] = MakePackedVertex(Mesh, static_cast<std::uint32_t>(CornerIndex));
    }

    std::vector<std::uint32_t> Remap{};
    Remap.resize(NumCorners);
    ufbx_vertex_stream Stream{};
    Stream.data = CornerVertices.data();
    Stream.vertex_count = NumCorners;
    Stream.vertex_size = sizeof(PackedVertex);
    ufbx_error GenErr{};
    const std::size_t UniqueVertexCount{ ufbx_generate_indices(&Stream, 1, Remap.data(), NumCorners, nullptr, &GenErr) };
    if (UniqueVertexCount == 0) {
        const std::string Desc{ (GenErr.description.data != nullptr) ? std::string{ GenErr.description.data, GenErr.description.length } : std::string{} };
        if (!Desc.empty()) {
            throw AssetError{ std::string{ "ufbx_generate_indices failed: " } + Desc };
        }
        throw AssetError{ "ufbx_generate_indices failed: unknown error" };
    }

    OutVertices.Reserve(OutVertices.VertexCount() + UniqueVertexCount);

    for (std::size_t VertexIndex{ 0 }; VertexIndex < UniqueVertexCount; ++VertexIndex) {
        const PackedVertex& Packed{ CornerVertices[VertexIndex] };
        OutVertices.Positions.push_back(glm::vec3{ Packed.Position[0], Packed.Position[1], Packed.Position[2] });
        OutVertices.Normals.push_back(glm::vec3{ Packed.Normal[0], Packed.Normal[1], Packed.Normal[2] });
        OutVertices.Colors.push_back(glm::vec4{ Packed.Color[0], Packed.Color[1], Packed.Color[2], Packed.Color[3] });
        OutVertices.Tangents.push_back(glm::vec3{ Packed.Tangent[0], Packed.Tangent[1], Packed.Tangent[2] });
        OutVertices.Bitangents.push_back(glm::vec3{ Packed.Bitangent[0], Packed.Bitangent[1], Packed.Bitangent[2] });
        OutVertices.BoneIndices.push_back(glm::uvec4{ Packed.BoneIndices[0], Packed.BoneIndices[1], Packed.BoneIndices[2], Packed.BoneIndices[3] });
        OutVertices.BoneWeights.push_back(glm::vec4{ Packed.BoneWeights[0], Packed.BoneWeights[1], Packed.BoneWeights[2], Packed.BoneWeights[3] });
        for (std::size_t SetIndex{ 0 }; SetIndex < 4; ++SetIndex) {
            OutVertices.TexCoords[SetIndex].push_back(glm::vec2{ Packed.TexCoord[SetIndex][0], Packed.TexCoord[SetIndex][1] });
        }
    }

    std::vector<std::uint32_t> TriCorners{};
    TriCorners.resize(static_cast<std::size_t>(Mesh.max_face_triangles) * 3);
    for (std::size_t FaceIndex{ 0 }; FaceIndex < Mesh.faces.count; ++FaceIndex) {
        const ufbx_face Face{ Mesh.faces.data[FaceIndex] };
        if (Face.num_indices < 3) {
            continue;
        }
        const std::uint32_t NumTris{ ufbx_triangulate_face(TriCorners.data(), TriCorners.size(), &Mesh, Face) };
        for (std::uint32_t TriIndex{ 0 }; TriIndex < NumTris; ++TriIndex) {
            const std::uint32_t Corner0{ TriCorners[TriIndex * 3 + 0] };
            const std::uint32_t Corner1{ TriCorners[TriIndex * 3 + 1] };
            const std::uint32_t Corner2{ TriCorners[TriIndex * 3 + 2] };
            OutIndices.push_back(Remap[Corner0]);
            OutIndices.push_back(Remap[Corner1]);
            OutIndices.push_back(Remap[Corner2]);
        }
    }
}
