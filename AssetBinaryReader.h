/*
Binary Format (v1):
- Header: char[4] Magic = "FBXB", uint32 Version = 1
- Materials: uint64 MaterialCount
  - Material: uint8 PbrFlag, uint64 PropertyCount
    - Property: uint16 Type, uint8 MapKind, Payload (by MapKind)
      - None: no payload
      - Real: float
      - Int: int64
      - Bool: uint8
      - Vec2: float[2]
      - Vec3: float[3]
      - Vec4: float[4]
      - String: uint64 Length, char[Length]
- Nodes (DFS order): uint64 NodeCount
  - Node: string Name, int32 ParentIndex (-1 root), mat4 NodeToParent, mat4 GeometryToNode
    - VertexAttributes: for each attribute vector, uint64 Count + raw bytes
      Positions(vec3), Normals(vec3), TexCoords[4](vec2), Colors(vec4),
      Tangents(vec3), Bitangents(vec3), BoneIndices(uvec4), BoneWeights(vec4)
    - Indices: uint64 Count + uint32[Count]
    - MaterialIndices: uint64 Count + uint64[Count]
*/
#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "AssetBundle.h"

namespace asset {
    class AssetBinaryReader final {
    public:
        AssetBinaryReader();
        ~AssetBinaryReader() = default;

        AssetBinaryReader(const AssetBinaryReader& Other) = delete;
        AssetBinaryReader& operator=(const AssetBinaryReader& Other) = delete;
        AssetBinaryReader(AssetBinaryReader&& Other) noexcept = delete;
        AssetBinaryReader& operator=(AssetBinaryReader&& Other) noexcept = delete;

    public:
        bool ReadFromFile(const std::string& Path, AssetBundle& Bundle);

    private:
        bool ReadHeader();
        void ReadMaterials(std::vector<Material>& Materials);
        Material ReadMaterial();
        MaterialProperty ReadMaterialProperty();
        MaterialMap ReadMaterialMap(MaterialMapKind Kind);
        void ReadModelResult(ModelResult& Result);
        void ReadNodes(ModelResult& Result, std::uint64_t NodeCount, std::vector<ModelNode*>& Nodes);
        void ReadVertexAttributes(VertexAttributes& Attributes);
        std::vector<glm::vec2> ReadVec2Array();
        std::vector<glm::vec3> ReadVec3Array();
        std::vector<glm::vec4> ReadVec4Array();
        std::vector<glm::uvec4> ReadUvec4Array();
        std::vector<std::uint32_t> ReadUint32Array();
        std::vector<std::uint64_t> ReadUint64Array();

        std::string ReadString();
        std::uint8_t ReadUint8();
        std::uint16_t ReadUint16();
        std::uint32_t ReadUint32();
        std::uint64_t ReadUint64();
        std::int32_t ReadInt32();
        std::int64_t ReadInt64();
        float ReadFloat();
        bool ReadBool();
        glm::vec2 ReadVec2();
        glm::vec3 ReadVec3();
        glm::vec4 ReadVec4();
        glm::mat4 ReadMat4();
        void ReadBytes(void* Data, std::size_t Size);

    private:
        std::ifstream mStream{};
    };
}
