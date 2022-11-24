#pragma once

#include <atomic>
#include <list>

#include <jbkvs/storageNode.h>

namespace jbkvs
{

    class Storage
        : detail::NonCopyableMixin<Storage>
    {
    public:
        struct MountPoint
        {
            std::string path;
            NodePtr node;

            MountPoint(const std::string_view& path, const NodePtr& node) : path(path), node(node) {}
        };

    private:
        mutable std::shared_mutex _mutex;
        uint32_t _mountPriorityCounter;
        std::list<MountPoint> _mountPoints;
        StorageNodePtr _root;

    public:
        Storage();
        ~Storage();

        bool mount(const std::string_view& path, const NodePtr& node);
        bool unmount(const std::string_view& path, const NodePtr& node);

        StorageNodePtr getNode(const std::string_view& path) const;
        std::vector<MountPoint> getMountPoints() const;

    private:
        void _mount(const std::string_view& path, const NodePtr& node, uint32_t priority);
        bool _unmount(const decltype(_mountPoints)::reverse_iterator& it);
    };

} // namespace jbkvs
