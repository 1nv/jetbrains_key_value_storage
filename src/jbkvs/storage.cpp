#include <jbkvs/storage.h>

namespace jbkvs
{

    Storage::Storage()
        : _mountPriorityCounter()
        , _root(StorageNode::_create())
    {
    }

    Storage::~Storage()
    {
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

        uint32_t priority = ++_mountPriorityCounter;

        detail::SubTreeLock subTreeLock(node);

        _root->_mountVirtual(path.substr(1), node, priority);
        return true;
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

        detail::SubTreeLock subTreeLock(node);

        StorageNode::_UnmountResult unmountResult = _root->_unmountVirtual(path.substr(1), node);
        return unmountResult.success;
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

        size_t end;
        for (size_t start = 1; start < length; start = end + 1)
        {
            end = path.find(StorageNode::_pathSeparator, start);

            if (end == std::string::npos)
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

} // namespace jbkvs
