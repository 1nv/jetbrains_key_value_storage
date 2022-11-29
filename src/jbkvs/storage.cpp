#include <jbkvs/storage.h>

#include <algorithm>
#include <iterator>

namespace jbkvs
{

    Storage::Storage()
        : _mutex()
        , _mountPriorityCounter()
        , _mountPoints()
        , _root(StorageNode::_create())
    {
    }

    Storage::~Storage()
    {
        std::unique_lock lock(_mutex);

        while (!_mountPoints.empty())
        {
            _unmount(_mountPoints.rbegin());
        }
    }

    bool Storage::mount(const std::string_view& path, const NodePtr& node)
    {
        if (path.empty() || path[0] != StorageNode::_pathSeparator)
        {
            return false;
        }

        if (!node)
        {
            return false;
        }

        char consecutiveSeparators[] = { StorageNode::_pathSeparator, StorageNode::_pathSeparator , 0 };
        if (path.find(consecutiveSeparators) != std::string_view::npos)
        {
            return false;
        }

        std::unique_lock lock(_mutex);

        uint32_t priority = ++_mountPriorityCounter;
        _mount(path, node, priority);

        return true;
    }

    void Storage::_mount(const std::string_view& path, const NodePtr& node, uint32_t priority)
    {
        detail::SubTreeLock subTreeLock(node);

        _root->_mountVirtual(path.substr(1), node, priority);

        _mountPoints.emplace_back(path, node);
    }

    bool Storage::unmount(const std::string_view& path, const NodePtr& node)
    {
        if (path.empty() || path[0] != StorageNode::_pathSeparator)
        {
            return false;
        }

        if (!node)
        {
            return false;
        }

        std::unique_lock lock(_mutex);

        auto it = std::find_if(_mountPoints.rbegin(), _mountPoints.rend(), [&path, &node](const MountPoint& mountPoint)
        {
            return mountPoint.path == path && mountPoint.node == node;
        });

        if (it == _mountPoints.rend())
        {
            return false;
        }

        _unmount(it);
        return true;
    }

    void Storage::_unmount(const decltype(_mountPoints)::reverse_iterator& it)
    {
        MountPoint mountPoint = std::move(*it);

        _mountPoints.erase(std::next(it).base());

        detail::SubTreeLock subTreeLock(mountPoint.node);

        _root->_unmountVirtual(std::string_view(mountPoint.path).substr(1), mountPoint.node);
    }

    StorageNodePtr Storage::getNode(const std::string_view& path) const
    {
        if (path.empty() || path[0] != StorageNode::_pathSeparator)
        {
            return StorageNodePtr();
        }

        size_t length = path.length();

        if (length == 1)
        {
            return _root;
        }

        StorageNodePtr current = _root;

        for (size_t start = 1, end; start < length; start = end + 1)
        {
            end = path.find(StorageNode::_pathSeparator, start);

            if (end == std::string_view::npos)
            {
                end = length;
            }

            current = current->getChild(path.substr(start, end - start));
            if (!current)
            {
                return StorageNodePtr();
            }
        }

        return current;
    }

    std::vector<Storage::MountPoint> Storage::getMountPoints() const
    {
        std::shared_lock lock(_mutex);

        std::vector<Storage::MountPoint> result;
        result.reserve(_mountPoints.size());
        std::copy(_mountPoints.begin(), _mountPoints.end(), std::back_inserter(result));
        return result;
    }

} // namespace jbkvs
