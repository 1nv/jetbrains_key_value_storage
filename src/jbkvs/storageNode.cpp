#include <jbkvs/storageNode.h>

#include <assert.h>

namespace jbkvs
{

    StorageNodePtr StorageNode::_create()
    {
        struct MakeSharedEnabledStorageNode : public StorageNode {};

        return std::make_shared<MakeSharedEnabledStorageNode>();
    }

    StorageNode::StorageNode()
        : _mutex()
        , _virtualMountCounter()
        , _mountedNodes()
        , _children()
    {
    }

    StorageNode::~StorageNode()
    {
    }

    StorageNodePtr StorageNode::getChild(const std::string& name) const
    {
        std::shared_lock lock(_mutex);

        auto it = _children.find(name);
        return (it != _children.end()) ? it->second : StorageNodePtr();
    }

    void StorageNode::_mountVirtual(const std::string& path, const NodePtr& node)
    {
        size_t length = path.length();

        if (length == 0)
        {
            return _mount(node, 0);
        }

        size_t end = path.find(_pathSeparator);
        if (end == std::string::npos)
        {
            end = length;
        }
        std::string childName = path.substr(0, end);

        std::unique_lock lock(_mutex);

        ++_virtualMountCounter;

        StorageNodePtr& child = _children[childName];
        if (!child)
        {
            child = _create();
        }

        std::string subPath = (end == length) ? std::string() : path.substr(end + 1);
        child->_mountVirtual(subPath, node);
    }

    bool StorageNode::_unmountVirtual(const std::string& path, const NodePtr& node)
    {
        size_t length = path.length();

        if (length == 0)
        {
            return _unmount(node, 0);
        }

        size_t end = path.find(_pathSeparator);
        if (end == std::string::npos)
        {
            end = length;
        }
        std::string childName = path.substr(0, end);

        std::unique_lock lock(_mutex);

        auto childIt = _children.find(childName);
        if (childIt == _children.end())
        {
            return false;
        }

        const StorageNodePtr& child = childIt->second;
        std::string subPath = (end == length) ? std::string() : path.substr(end + 1);
        bool unmounted = child->_unmountVirtual(subPath, node);

        if (unmounted)
        {
            if (child->_isReadyForEviction())
            {
                _children.erase(childIt);
            }

            assert(_virtualMountCounter > 0);
            --_virtualMountCounter;
        }

        return unmounted;
    }

    void StorageNode::_mount(const NodePtr& node, size_t depth)
    {
        std::unique_lock lock(_mutex);

        _mountedNodes.emplace_back(node, depth);

        for (const auto& [childName, nodeChild] : node->getChildren())
        {
            StorageNodePtr& child = _children[childName];
            if (!child)
            {
                child = _create();
            }

            child->_mount(nodeChild, depth + 1);
        }
    }

    bool StorageNode::_unmount(const NodePtr& node, size_t depth)
    {
        std::unique_lock lock(_mutex);

        auto it = std::find_if(_mountedNodes.begin(), _mountedNodes.end(), [&](const MountedNode& mountedNode)
            {
                return mountedNode.node == node && mountedNode.depth == depth;
            });

        if (it == _mountedNodes.end())
        {
            assert(depth == 0);
            return false;
        }

        for (const auto& [childName, childNode] : node->getChildren())
        {
            auto childIt = _children.find(childName);
            assert(childIt != _children.end());

            const StorageNodePtr& child = childIt->second;
            bool unmounted = child->_unmount(childNode, depth + 1);
            assert(unmounted);

            if (child->_isReadyForEviction())
            {
                _children.erase(childIt);
            }
        }

        _mountedNodes.erase(it);

        return true;
    }

    bool StorageNode::_isReadyForEviction() const
    {
        return _virtualMountCounter == 0 && _mountedNodes.empty();
    }

} // namespace jbkvs
