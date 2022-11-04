#pragma once

#include <jbkvs/storageNode.h>

namespace jbkvs
{

    class Storage
        : detail::NonCopyableMixin<Storage>
    {
        StorageNodePtr _root;

    public:
        Storage();
        ~Storage();

        bool mount(const std::string& path, const NodePtr& node);
        bool unmount(const std::string& path, const NodePtr& node);

        StorageNodePtr getNode(const std::string& path) const;
    };

} // namespace jbkvs
