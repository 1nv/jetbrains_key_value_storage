#include <jbkvs/storage.h>

namespace jbkvs
{

    Storage::Storage()
        : _root(StorageNode::_create())
    {
    }

    Storage::~Storage()
    {
    }

    bool Storage::mount(const std::string& path, const NodePtr& node)
    {
        if (path.empty() || path[0] != StorageNode::_pathSeparator)
        {
            return false;
        }

        if (!node)
        {
            return false;
        }

        _root->_mountVirtual(path.substr(1), node);
        return true;
    }

    bool Storage::unmount(const std::string& path, const NodePtr& node)
    {
        if (path.empty() || path[0] != StorageNode::_pathSeparator)
        {
            return false;
        }

        if (!node)
        {
            return false;
        }

        StorageNode::_UnmountResult unmountResult = _root->_unmountVirtual(path.substr(1), node);
        return unmountResult.success;
    }

    StorageNodePtr Storage::getNode(const std::string& path) const
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

        size_t start = 1;
        size_t end = path.find(StorageNode::_pathSeparator, start);
        while (true)
        {
            if (end == std::string::npos)
            {
                end = length;
            }

            current = current->getChild(path.substr(start, end - start));
            if (!current)
            {
                return StorageNodePtr();
            }

            if (end >= length - 1)
            {
                break;
            }

            start = end + 1;
            end = path.find(StorageNode::_pathSeparator, start);
        }

        return current;
    }

} // namespace jbkvs
