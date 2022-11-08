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

    void StorageNode::_mountVirtual(const std::string_view& path, const NodePtr& node)
    {
        size_t length = path.length();

        if (length == 0)
        {
            return _mount(node, 0);
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
        child->_mountVirtual(subPath, node);
    }

    StorageNode::_UnmountResult StorageNode::_unmountVirtual(const std::string_view& path, const NodePtr& node)
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
        if (childIt == _children.end())
        {
            return {};
        }

        const StorageNodePtr& child = childIt->second;
        std::string_view subPath = (end == length) ? std::string_view() : path.substr(end + 1);
        _UnmountResult childUnmountResult = child->_unmountVirtual(subPath, node);

        if (!childUnmountResult.success)
        {
            return {};
        }

        if (childUnmountResult.detach)
        {
            _children.erase(childIt);
        }

        assert(_virtualMountCounter > 0);
        --_virtualMountCounter;

        _UnmountResult result = {};
        result.success = true;
        if (_isReadyForDetach())
        {
            result.detach = true;
        }

        return result;
    }

    void StorageNode::_mount(const NodePtr& node, size_t depth)
    {
        std::unique_lock lock(_mutex);

        node->_onMounting();

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

    StorageNode::_UnmountResult StorageNode::_unmount(const NodePtr& node, size_t depth)
    {
        std::unique_lock lock(_mutex);

        auto it = std::find_if(_mountedNodes.rbegin(), _mountedNodes.rend(), [&](const MountedNode& mountedNode)
            {
                return mountedNode.node == node && mountedNode.depth == depth;
            });

        if (it == _mountedNodes.rend())
        {
            assert(depth == 0);
            return {};
        }

        for (const auto& [childName, childNode] : node->getChildren())
        {
            auto childIt = _children.find(childName);
            assert(childIt != _children.end());

            const StorageNodePtr& child = childIt->second;
            _UnmountResult childUnmountResult = child->_unmount(childNode, depth + 1);
            assert(childUnmountResult.success);

            if (childUnmountResult.detach)
            {
                _children.erase(childIt);
            }
        }

        _mountedNodes.erase(std::next(it).base());

        node->_onUnmounted();

        _UnmountResult result = {};
        result.success = true;
        if (_isReadyForDetach())
        {
            result.detach = true;
        }

        return result;
    }

    bool StorageNode::_isReadyForDetach() const
    {
        return _virtualMountCounter == 0 && _mountedNodes.empty();
    }

} // namespace jbkvs
