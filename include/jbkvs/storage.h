#pragma once

#include <atomic>

#include <jbkvs/storageNode.h>

namespace jbkvs
{

    class Storage
        : detail::NonCopyableMixin<Storage>
    {
        std::atomic<uint32_t> _mountPriorityCounter;
        StorageNodePtr _root;

    public:
        Storage();
        ~Storage();

        bool mount(const std::string_view& path, const NodePtr& node);
        bool unmount(const std::string_view& path, const NodePtr& node);

        StorageNodePtr getNode(const std::string_view& path) const;
    };

} // namespace jbkvs
