#pragma once

#include "Common.h"
#include "DataList.h"

namespace asset {
    class ModelNode final {
    public:
        using Id = std::uint32_t;

    public:
        ModelNode(Id IdValue, std::string Name);
        ~ModelNode() = default;

        ModelNode(const ModelNode&) = delete;
        ModelNode& operator=(const ModelNode&) = delete;
        ModelNode(ModelNode&&) = default;
        ModelNode& operator=(ModelNode&&) = default;

    public:
        Id GetId() const;
        const std::string& GetName() const;

        const glm::mat4& GetNodeToParent() const;
        void SetNodeToParent(const glm::mat4& NodeToParent);

        const glm::mat4& GetGeometryToNode() const;
        void SetGeometryToNode(const glm::mat4& GeometryToNode);

        ModelNode* GetParent() const;
        const std::vector<ModelNode*>& GetChildren() const;
        void AddChild(ModelNode* Child);

        DataList& Vertices();
        const DataList& Vertices() const;

        DataList& Indices();
        const DataList& Indices() const;

        std::vector<const ModelNode*> GetChildChain() const;

    private:
        Id mId{ 0 };

        std::string mName{};
        glm::mat4 mNodeToParent{ 1.0f };
        glm::mat4 mGeometryToNode{ 1.0f };

        ModelNode* mParent{ nullptr };
        std::vector<ModelNode*> mChildren{};

        DataList mVertices{};
        DataList mIndices{};
    };

    class ModelResult final {
    public:
        ModelResult();
        ~ModelResult() = default;

        ModelResult(const ModelResult&) = delete;
        ModelResult& operator=(const ModelResult&) = delete;
        ModelResult(ModelResult&&) = default;
        ModelResult& operator=(ModelResult&&) = default;

    public:
        ModelNode* GetRoot() const;
        std::size_t NodeCount() const;
        const std::vector<std::unique_ptr<ModelNode>>& Nodes() const;

        ModelNode& CreateNode(std::string Name, ModelNode* Parent);
        void ForEachDFS(const std::function<void(ModelNode&)>& Function) const;

    private:
        std::vector<std::unique_ptr<ModelNode>> mNodes{};
        ModelNode* mRoot{ nullptr };
        std::uint64_t mNextId{ 1 };
    };
}