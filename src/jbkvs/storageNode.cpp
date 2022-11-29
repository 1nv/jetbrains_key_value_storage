#include <jbkvs/storageNode.h>

#include <assert.h>
#include <algorithm>

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

    StorageNodePtr StorageNode::getChild(const std::string_view& name) const
    {
        std::shared_lock lock(_mutex);

        auto it = _children.find(name);
        return (it != _children.end()) ? it->second : StorageNodePtr();
    }

    void StorageNode::_mountVirtual(const std::string_view& path, const NodePtr& node, uint32_t priority)
    {
        size_t length = path.length();

        if (length == 0)
        {
            return _mount(node, 0, priority);
        }

        size_t end = path.find(_pathSeparator);
        if (end == std::string_view::npos)
        {
            end = length;
        }
        std::string childName = std::string(path.substr(0, end));

        std::unique_lock lock(_mutex);

        ++_virtualMountCounter;

        StorageNodePtr& child = _children[std::move(childName)];
        if (!child)
        {
            child = _create();
        }

        std::string_view subPath = (end == length) ? std::string_view() : path.substr(end + 1);
        child->_mountVirtual(subPath, node, priority);
    }

    bool StorageNode::_unmountVirtual(const std::string_view& path, const NodePtr& node)
    {
        size_t length = path.length();

        if (length == 0)
        {
            return _unmount(node, 0);
        }

        size_t end = path.find(_pathSeparator);
        if (end == std::string_view::npos)
        {
            end = length;
        }
        std::string_view childName = path.substr(0, end);

        std::unique_lock lock(_mutex);

        auto childIt = _children.find(childName);
        assert(childIt != _children.end());

        const StorageNodePtr& child = childIt->second;
        std::string_view subPath = (end == length) ? std::string_view() : path.substr(end + 1);
        bool detachChild = child->_unmountVirtual(subPath, node);

        if (detachChild)
        {
            _children.erase(childIt);
        }

        assert(_virtualMountCounter > 0);
        --_virtualMountCounter;

        bool detach = _isReadyForDetach();
        return detach;
    }

    void StorageNode::_mount(const NodePtr& node, size_t depth, uint32_t priority)
    {
        std::unique_lock lock(_mutex);

        node->_onMounting(this, depth, priority);

        auto it = std::lower_bound(_mountedNodes.begin(), _mountedNodes.end(), priority, [](const MountedNode& mountedNode, uint32_t p)
        {
            return mountedNode.priority < p;
        });

        _mountedNodes.emplace(it, node, depth, priority);

        for (const auto& [childName, childNode] : node->_children)
        {
            StorageNodePtr& child = _children[childName];
            if (!child)
            {
                child = _create();
            }

            child->_mount(childNode, depth + 1, priority);
        }
    }

    bool StorageNode::_unmount(const NodePtr& node, size_t depth)
    {
        std::unique_lock lock(_mutex);

        auto it = std::find_if(_mountedNodes.rbegin(), _mountedNodes.rend(), [&](const MountedNode& mountedNode)
        {
            return mountedNode.node == node && mountedNode.depth == depth;
        });
        assert(it != _mountedNodes.rend());

        for (auto it = node->_children.rbegin(); it != node->_children.rend(); ++it)
        {
            const auto& [childName, childNode] = *it;

            auto childIt = _children.find(childName);
            assert(childIt != _children.end());

            const StorageNodePtr& child = childIt->second;
            bool detachChild = child->_unmount(childNode, depth + 1);

            if (detachChild)
            {
                _children.erase(childIt);
            }
        }

        _mountedNodes.erase(std::next(it).base());

        node->_onUnmounted(this, depth);

        bool detach = _isReadyForDetach();
        return detach;
    }

    void StorageNode::_attachMountedNodeChild(size_t depth, uint32_t priority, const std::string& childName, const NodePtr& childNode)
    {
        std::unique_lock lock(_mutex);

        StorageNodePtr& child = _children[childName];
        if (!child)
        {
            child = _create();
        }

        child->_mount(childNode, depth + 1, priority);
    }

    void StorageNode::_detachMountedNodeChild(size_t depth, const std::string& childName, const NodePtr& childNode)
    {
        std::unique_lock lock(_mutex);

        auto childIt = _children.find(childName);
        assert(childIt != _children.end());

        bool detachChild = childIt->second->_unmount(childNode, depth + 1);

        if (detachChild)
        {
            _children.erase(childIt);
        }
    }

    bool StorageNode::_isReadyForDetach() const noexcept
    {
        return _virtualMountCounter == 0 && _mountedNodes.empty();
    }

} // namespace jbkvs
