#include "MeshHierarchyBuilder.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

using namespace asset;

MeshHierarchyBuilder::MeshHierarchyBuilder(ModelResult& OutResult)
    : mResult{ OutResult } {
}

MeshHierarchyBuilder::~MeshHierarchyBuilder() = default;

void MeshHierarchyBuilder::OnNodeBegin(const ufbx_scene& Scene, const ufbx_node& Node, const NodeVisitContext& Ctx) {
    (void)Scene;
    ModelNode* ParentNode{ nullptr };
    if (!mNodeStack.empty()) {
        ParentNode = mNodeStack.back();
    }

    const std::string Name{ (Node.name.data != nullptr) ? std::string{ Node.name.data, Node.name.length } : std::string{ "Unnamed" } };

    ModelNode& OutNode{ mResult.CreateNode(Name, ParentNode) };
    OutNode.SetNodeToParent(Ctx.mNodeToParent);
    OutNode.SetGeometryToNode(Ctx.mGeometryToNode);
    if (Node.mesh != nullptr) {
        AppendIndexedMeshUfbx(*Node.mesh, OutNode.Vertices(), OutNode.Indices());
    }
    mNodeStack.push_back(&OutNode);
}

void MeshHierarchyBuilder::OnNodeEnd(const ufbx_scene& Scene, const ufbx_node& Node) {
    (void)Scene;
    (void)Node;
    if (!mNodeStack.empty()) {
        mNodeStack.pop_back();
        return;
    }
    throw AssetError{ "MeshHierarchyBuilder: node stack underflow" };
}

glm::vec3 MeshHierarchyBuilder::ToGlmVec3(const ufbx_vec3& V) {
    return glm::vec3{ static_cast<float>(V.x), static_cast<float>(V.y), static_cast<float>(V.z) };
}

glm::vec2 MeshHierarchyBuilder::ToGlmVec2(const ufbx_vec2& V) {
    return glm::vec2{ static_cast<float>(V.x), static_cast<float>(V.y) };
}

glm::vec4 MeshHierarchyBuilder::ToGlmVec4(const ufbx_vec4& V) {
    return glm::vec4{ static_cast<float>(V.x), static_cast<float>(V.y), static_cast<float>(V.z), static_cast<float>(V.w) };
}

glm::vec3 MeshHierarchyBuilder::ReadPosition(const ufbx_mesh& Mesh, std::uint32_t Index) const {
    const ufbx_vec3 V{ ufbx_get_vertex_vec3(&Mesh.vertex_position, Index) };
    return ToGlmVec3(V);
}

glm::vec3 MeshHierarchyBuilder::ReadNormal(const ufbx_mesh& Mesh, std::uint32_t Index) const {
    if (Mesh.vertex_normal.exists) {
        const ufbx_vec3 V{ ufbx_get_vertex_vec3(&Mesh.vertex_normal, Index) };
        return glm::normalize(ToGlmVec3(V));
    }
    return glm::vec3{ 0.0f, 1.0f, 0.0f };
}

glm::vec2 MeshHierarchyBuilder::ReadTexCoord(const ufbx_mesh& Mesh, std::uint32_t Index) const {
    if (Mesh.vertex_uv.exists) {
        const ufbx_vec2 V{ ufbx_get_vertex_vec2(&Mesh.vertex_uv, Index) };
        return ToGlmVec2(V);
    }
    return glm::vec2{ 0.0f, 0.0f };
}

glm::vec4 MeshHierarchyBuilder::ReadColor(const ufbx_mesh& Mesh, std::uint32_t Index) const {
    if (Mesh.vertex_color.exists) {
        const ufbx_vec4 V{ ufbx_get_vertex_vec4(&Mesh.vertex_color, Index) };
        return ToGlmVec4(V);
    }
    return glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
}

MeshHierarchyBuilder::PackedVertex MeshHierarchyBuilder::MakePackedVertex(const ufbx_mesh& Mesh, std::uint32_t CornerIndex) const {
    PackedVertex Pv{};
    const glm::vec3 P{ ReadPosition(Mesh, CornerIndex) };
    const glm::vec3 N{ ReadNormal(Mesh, CornerIndex) };
    const glm::vec2 Uv{ ReadTexCoord(Mesh, CornerIndex) };
    const glm::vec4 C{ ReadColor(Mesh, CornerIndex) };
    Pv.Position[0] = P.x; Pv.Position[1] = P.y; Pv.Position[2] = P.z;
    Pv.Normal[0] = N.x; Pv.Normal[1] = N.y; Pv.Normal[2] = N.z;
    Pv.TexCoord[0] = Uv.x; Pv.TexCoord[1] = Uv.y;
    Pv.Color[0] = C.x; Pv.Color[1] = C.y; Pv.Color[2] = C.z; Pv.Color[3] = C.w;
    return Pv;
}

void MeshHierarchyBuilder::AppendIndexedMeshUfbx(const ufbx_mesh& Mesh, DataList& OutVertices, DataList& OutIndices) const {
    const std::size_t NumCorners{ Mesh.num_indices };
    if (NumCorners == 0) {
        return;
    }
    std::vector<PackedVertex> CornerVertices{};
    CornerVertices.resize(NumCorners);
    for (std::size_t Ci{ 0 }; Ci < NumCorners; ++Ci) {
        CornerVertices[Ci] = MakePackedVertex(Mesh, static_cast<std::uint32_t>(Ci));
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
    for (std::size_t Vi{ 0 }; Vi < UniqueVertexCount; ++Vi) {
        const PackedVertex& Pv{ CornerVertices[Vi] };
        Vertex V{};
        V.Position = glm::vec3{ Pv.Position[0], Pv.Position[1], Pv.Position[2] };
        V.Normal = glm::vec3{ Pv.Normal[0], Pv.Normal[1], Pv.Normal[2] };
        V.TexCoord = glm::vec2{ Pv.TexCoord[0], Pv.TexCoord[1] };
        V.Color = glm::vec4{ Pv.Color[0], Pv.Color[1], Pv.Color[2], Pv.Color[3] };
        OutVertices.PushBack<Vertex>(V);
    }
    std::vector<std::uint32_t> TriCorners{};
    TriCorners.resize(static_cast<std::size_t>(Mesh.max_face_triangles) * 3);
    for (std::size_t FaceIndex{ 0 }; FaceIndex < Mesh.faces.count; ++FaceIndex) {
        const ufbx_face Face{ Mesh.faces.data[FaceIndex] };
        if (Face.num_indices < 3) {
            continue;
        }
        const std::uint32_t NumTris{ ufbx_triangulate_face(TriCorners.data(), TriCorners.size(), &Mesh, Face) };
        for (std::uint32_t T{ 0 }; T < NumTris; ++T) {
            const std::uint32_t C0{ TriCorners[T * 3 + 0] };
            const std::uint32_t C1{ TriCorners[T * 3 + 1] };
            const std::uint32_t C2{ TriCorners[T * 3 + 2] };
            OutIndices.PushBack<std::uint32_t>(Remap[C0]);
            OutIndices.PushBack<std::uint32_t>(Remap[C1]);
            OutIndices.PushBack<std::uint32_t>(Remap[C2]);
        }
    }
}