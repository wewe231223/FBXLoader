#include "ModelResult.h"
#include <algorithm>

using namespace asset;

ModelNode::ModelNode(Id IdValue, std::string Name)
    : mId{ IdValue },
    mName{ std::move(Name) } {
}

ModelNode::Id ModelNode::GetId() const {
    return mId;
}

const std::string& ModelNode::GetName() const {
    return mName;
}

const glm::mat4& ModelNode::GetNodeToParent() const {
    return mNodeToParent;
}

void ModelNode::SetNodeToParent(const glm::mat4& NodeToParent) {
    mNodeToParent = NodeToParent;
}

const glm::mat4& ModelNode::GetGeometryToNode() const {
    return mGeometryToNode;
}

void ModelNode::SetGeometryToNode(const glm::mat4& GeometryToNode) {
    mGeometryToNode = GeometryToNode;
}

ModelNode* ModelNode::GetParent() const {
    return mParent;
}

const std::vector<ModelNode*>& ModelNode::GetChildren() const {
    return mChildren;
}

void ModelNode::AddChild(ModelNode* Child) {
    assert(Child != nullptr);
    mChildren.push_back(Child);
    Child->mParent = this;
}

DataList& ModelNode::Vertices() {
    return mVertices;
}

const DataList& ModelNode::Vertices() const {
    return mVertices;
}

DataList& ModelNode::Indices() {
    return mIndices;
}

const DataList& ModelNode::Indices() const {
    return mIndices;
}

std::vector<const ModelNode*> ModelNode::GetChildChain() const {
    std::vector<const ModelNode*> Chain{};
    const ModelNode* Current{ this };
    while (Current != nullptr) {
        Chain.push_back(Current);
        Current = Current->GetParent();
    }
    std::reverse(Chain.begin(), Chain.end());
    return Chain;
}

ModelResult::ModelResult() = default;

ModelNode* ModelResult::GetRoot() const {
    return mRoot;
}

std::size_t ModelResult::NodeCount() const {
    return mNodes.size();
}

const std::vector<std::unique_ptr<ModelNode>>& ModelResult::Nodes() const {
    return mNodes;
}

ModelNode& ModelResult::CreateNode(std::string Name, ModelNode* Parent) {
    const ModelNode::Id IdValue{ static_cast<ModelNode::Id>(mNextId) };
    mNextId += 1;
    mNodes.push_back(std::make_unique<ModelNode>(IdValue, std::move(Name)));
    ModelNode& Node{ *mNodes.back() };
    if (Parent != nullptr) {
        Parent->AddChild(&Node);
    }
    else if (mRoot == nullptr) {
        mRoot = &Node;
    }
    return Node;
}

void ModelResult::ForEachDFS(const std::function<void(ModelNode&)>& Function) const {
    if (mRoot == nullptr) {
        return;
    }
    std::vector<ModelNode*> Stack{};
    Stack.push_back(mRoot);
    while (!Stack.empty()) {
        ModelNode* Node{ Stack.back() };
        Stack.pop_back();
        Function(*Node);
        const std::vector<ModelNode*>& Children{ Node->GetChildren() };
        for (std::size_t I{ Children.size() }; I > 0; --I) {
            Stack.push_back(Children[I - 1]);
        }
    }
}