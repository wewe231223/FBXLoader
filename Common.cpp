#include "Common.h"

using namespace asset;

std::size_t VertexAttributes::VertexCount() const {
    return Positions.size();
}

bool VertexAttributes::Empty() const {
    return Positions.empty();
}

void VertexAttributes::Reserve(std::size_t Count) {
    Positions.reserve(Count);
    Normals.reserve(Count);
    Colors.reserve(Count);
    Tangents.reserve(Count);
    Bitangents.reserve(Count);
    BoneIndices.reserve(Count);
    BoneWeights.reserve(Count);
    for (std::vector<glm::vec2>& Set : TexCoords) {
        Set.reserve(Count);
    }
}

void VertexAttributes::Resize(std::size_t Count) {
    Positions.resize(Count);
    Normals.resize(Count);
    Colors.resize(Count);
    Tangents.resize(Count);
    Bitangents.resize(Count);
    BoneIndices.resize(Count);
    BoneWeights.resize(Count);
    for (std::vector<glm::vec2>& Set : TexCoords) {
        Set.resize(Count);
    }
}
