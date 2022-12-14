#pragma once

#include <vector>

#include <jbkvs/node.h>

namespace jbkvs
{

    typedef std::shared_ptr<class StorageNode> StorageNodePtr;

    class StorageNode
        : public detail::NonCopyableMixin<StorageNode>
    {
        friend class Storage;
        friend class Node;

        static inline const char _pathSeparator = '/';

        struct MountedNode
        {
            NodePtr node;
            size_t depth;
            uint32_t priority;

            MountedNode(const NodePtr& node, size_t depth, uint32_t priority) : node(node), depth(depth), priority(priority) {}
        };

        // TODO: try lock-free approach.
        mutable std::shared_mutex _mutex;

        size_t _virtualMountCounter;
        std::vector<MountedNode> _mountedNodes;
        std::map<std::string, StorageNodePtr, std::less<>> _children;

    public:
        template <typename T>
        std::optional<T> get(const TKey& key) const
        {
            std::shared_lock lock(_mutex);

            std::optional<T> result;
            for (size_t i = _mountedNodes.size() - 1; ~i; --i)
            {
                result = _mountedNodes[i].node->get<T>(key);
                if (result)
                {
                    return result;
                }
            }
            return result;
        }

        StorageNodePtr getChild(const std::string_view& name) const;

    private:
        static StorageNodePtr _create();

        StorageNode();
        ~StorageNode();

        void _mountVirtual(const std::string_view& path, const NodePtr& node, uint32_t priority);
        bool _unmountVirtual(const std::string_view& path, const NodePtr& node);
        void _mount(const NodePtr& node, size_t depth, uint32_t priority);
        bool _unmount(const NodePtr& node, size_t depth);

        void _attachMountedNodeChild(size_t depth, uint32_t priority, const std::string& childName, const NodePtr& childNode);
        void _detachMountedNodeChild(size_t depth, const std::string& childName, const NodePtr& childNode);

        bool _isReadyForDetach() const noexcept;
    };

} // namespace jbkvs
