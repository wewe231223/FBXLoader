#pragma once

#include <unordered_set>
#include <vector>

#include "Common.h"
#include "SceneVisitor.h"

namespace asset {
    class MaterialVisitor final : public ISceneNodeVisitor {
    public:
        MaterialVisitor();
        ~MaterialVisitor();

        MaterialVisitor(const MaterialVisitor& Other) = delete;
        MaterialVisitor& operator=(const MaterialVisitor& Other) = delete;
        MaterialVisitor(MaterialVisitor&& Other) = delete;
        MaterialVisitor& operator=(MaterialVisitor&& Other) = delete;

    public:
        void OnNodeBegin(const ufbx_scene& Scene, const ufbx_node& Node, const NodeVisitContext& Context) override;
        void OnNodeEnd(const ufbx_scene& Scene, const ufbx_node& Node) override;

        const std::vector<Material>& GetMaterials() const;
        void Clear();

    private:
        std::vector<Material> mMaterials{};
        std::unordered_set<const ufbx_material*> mVisitedMaterials{};
    };
}
