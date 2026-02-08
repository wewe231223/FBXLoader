#include "UfbxAssetLoader.h"

#include <string>

using namespace asset;

namespace {
    std::string ToString(const ufbx_string& String) {
        if (String.data == nullptr) {
            return std::string{};
        }
        return std::string{ String.data, String.length };
    }
}

UfbxAssetLoader::SceneHandle::SceneHandle()
    : mScene{ nullptr } {
}

UfbxAssetLoader::SceneHandle::SceneHandle(ufbx_scene* Scene)
    : mScene{ Scene } {
}

UfbxAssetLoader::SceneHandle::SceneHandle(SceneHandle&& Other) noexcept
    : mScene{ Other.mScene } {
    Other.mScene = nullptr;
}

UfbxAssetLoader::SceneHandle& UfbxAssetLoader::SceneHandle::operator=(SceneHandle&& Other) noexcept {
    if (this != &Other) {
        Reset();
        mScene = Other.mScene;
        Other.mScene = nullptr;
    }
    return *this;
}

UfbxAssetLoader::SceneHandle::~SceneHandle() {
    Reset();
}

ufbx_scene* UfbxAssetLoader::SceneHandle::GetScene() const {
    return mScene;
}

void UfbxAssetLoader::SceneHandle::Reset() {
    if (mScene != nullptr) {
        ufbx_free_scene(mScene);
        mScene = nullptr;
    }
}

UfbxAssetLoader::UfbxAssetLoader(GraphicsAPI Api)
    : mApi{ Api } {
}

glm::mat4 UfbxAssetLoader::ToGlmMat4(const ufbx_matrix& Matrix) {
    glm::mat4 Out{ 1.0f };
    Out[0] = glm::vec4{ static_cast<float>(Matrix.cols[0].x), static_cast<float>(Matrix.cols[0].y), static_cast<float>(Matrix.cols[0].z), 0.0f };
    Out[1] = glm::vec4{ static_cast<float>(Matrix.cols[1].x), static_cast<float>(Matrix.cols[1].y), static_cast<float>(Matrix.cols[1].z), 0.0f };
    Out[2] = glm::vec4{ static_cast<float>(Matrix.cols[2].x), static_cast<float>(Matrix.cols[2].y), static_cast<float>(Matrix.cols[2].z), 0.0f };
    Out[3] = glm::vec4{ static_cast<float>(Matrix.cols[3].x), static_cast<float>(Matrix.cols[3].y), static_cast<float>(Matrix.cols[3].z), 1.0f };
    return Out;
}

void UfbxAssetLoader::LoadAndTraverse(std::string_view FilePath, std::span<ISceneNodeVisitor* const> Visitors) {
    ufbx_load_opts Opts{};
    if (mApi == GraphicsAPI::DirectX) {
        Opts.target_axes = ufbx_axes_left_handed_y_up;
    }
    else {
        Opts.target_axes = ufbx_axes_right_handed_y_up;
    }
    Opts.target_unit_meters = 1.0;
    ufbx_error Error{};
    ufbx_scene* Scene{ ufbx_load_file(std::string{ FilePath }.c_str(), &Opts, &Error) };
    if (Scene == nullptr) {
        std::string Description{ ToString(Error.description) };
        if (!Description.empty()) {
            throw AssetError{ std::string{ "ufbx_load_file failed: " } + Description };
        }
        throw AssetError{ "ufbx_load_file failed: unknown error" };
    }
    SceneHandle Handle{ Scene };
    if (Handle.GetScene()->root_node != nullptr) {
        TraverseNode(*Handle.GetScene(), *Handle.GetScene()->root_node, nullptr, Visitors);
        return;
    }
    for (std::size_t Index{ 0 }; Index < Handle.GetScene()->nodes.count; ++Index) {
        const ufbx_node* Node{ Handle.GetScene()->nodes.data[Index] };
        if (Node != nullptr) {
            TraverseNode(*Handle.GetScene(), *Node, Node->parent, Visitors);
        }
    }
}

void UfbxAssetLoader::TraverseNode(const ufbx_scene& Scene, const ufbx_node& Node, const ufbx_node* Parent, std::span<ISceneNodeVisitor* const> Visitors) {
    NodeVisitContext Context{};
    Context.mParent = Parent;
    Context.mNodeToParent = ToGlmMat4(Node.node_to_parent);
    if (Node.has_geometry_transform) {
        Context.mGeometryToNode = ToGlmMat4(Node.geometry_to_node);
    }
    else {
        Context.mGeometryToNode = glm::mat4{ 1.0f };
    }
    for (ISceneNodeVisitor* Visitor : Visitors) {
        if (Visitor != nullptr) {
            Visitor->OnNodeBegin(Scene, Node, Context);
        }
    }
    for (std::size_t Index{ 0 }; Index < Node.children.count; ++Index) {
        const ufbx_node* Child{ Node.children.data[Index] };
        if (Child != nullptr) {
            TraverseNode(Scene, *Child, &Node, Visitors);
        }
    }
    for (ISceneNodeVisitor* Visitor : Visitors) {
        if (Visitor != nullptr) {
            Visitor->OnNodeEnd(Scene, Node);
        }
    }
}