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

        static inline const char _pathSeparator = '/';

        struct MountedNode
        {
            NodePtr node;
            size_t depth;

            MountedNode(const NodePtr& node, size_t depth) : node(node), depth(depth) {}
        };

        // TODO: try lock-free approach.
        mutable std::shared_mutex _mutex;

        size_t _virtualMountCounter;
        std::vector<MountedNode> _mountedNodes;
        std::map<std::string, StorageNodePtr> _children;

    public:
        template <typename T>
        std::optional<T> get(const TKey& key) const
        {
            std::shared_lock lock(_mutex);

            std::optional<T> result;
            for (const MountedNode& mountedNode : _mountedNodes)
            {
                result = mountedNode.node->get<T>(key);
                if (result)
                {
                    return result;
                }
            }
            return result;
        }

        StorageNodePtr getChild(const std::string& name) const;

    private:
        static StorageNodePtr _create();

        StorageNode();
        ~StorageNode();

        void _mountVirtual(const std::string& path, const NodePtr& node);
        bool _unmountVirtual(const std::string& path, const NodePtr& node);
        void _mount(const NodePtr& node, size_t depth);
        bool _unmount(const NodePtr& node, size_t depth);

        bool _isReadyForEviction() const;
    };

} // namespace jbkvs
