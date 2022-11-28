#include <jbkvs/node.h>
#include <jbkvs/storageNode.h>

#include <assert.h>
#include <algorithm>

namespace jbkvs
{

    namespace detail
    {

        SubTreeLock::SubTreeLock(const NodePtr& node)
            : _node(node)
        {
            _node->_lockSubTree();
        }

        SubTreeLock::~SubTreeLock()
        {
            _node->_unlockSubTree();
        }

    } // namespace detail

    NodePtr Node::create()
    {
        return create({}, {});
    }

    NodePtr Node::create(const NodePtr& parent, const std::string_view& name)
    {
        struct MakeSharedEnabledNode : public Node
        {
            MakeSharedEnabledNode(const NodePtr& parent, const std::string_view& name)
                : Node(parent, name)
            {
            }
        };

        NodePtr newNode = std::make_shared<MakeSharedEnabledNode>(parent, name);
        if (parent)
        {
            parent->_attachChild(newNode->_name, newNode);
        }

        return newNode;
    }

    Node::Node(const NodePtr& parent, const std::string_view& name)
        : _parent(parent)
        , _name(name)
        , _mutex()
        , _mountPoints()
        , _children()
        , _data()
    {
    }

    Node::~Node()
    {
        _children.clear();
    }

    void Node::_lockSubTree()
    {
        _mutex.lock();

        for (auto it = _children.begin(); it != _children.end(); ++it)
        {
            it->second->_lockSubTree();
        }
    }

    void Node::_unlockSubTree()
    {
        for (auto it = _children.rbegin(); it != _children.rend(); ++it)
        {
            it->second->_unlockSubTree();
        }

        _mutex.unlock();
    }

    bool Node::detach()
    {
        NodePtr parent;

        {
            std::unique_lock lock(_mutex);
            parent = _parent.lock();
            _parent.reset();
        }

        if (parent)
        {
            bool detached = parent->_detachChild(_name);
            return detached;
        }
        else
        {
            return false;
        }
    }

    NodePtr Node::getChild(const std::string_view& name) const
    {
        std::shared_lock lock(_mutex);

        auto it = _children.find(name);
        return (it != _children.end()) ? it->second : NodePtr();
    }

    void Node::_attachChild(const std::string& name, const NodePtr& child)
    {
        std::unique_lock lock(_mutex);

        // No need to lock on child, as we only attach newly created nodes that haven't been announced elsewhere.

        _children[name] = child;

        for (const MountPoint& mountPoint : _mountPoints)
        {
            mountPoint.storageNode->_attachMountedNodeChild(mountPoint.depth, mountPoint.priority, name, child);
        }
    }

    bool Node::_detachChild(const std::string& name)
    {
        std::unique_lock lock(_mutex);

        // TODO: think if it is better to use shared_from_this().
        auto childIt = _children.find(name);
        if (childIt == _children.end())
        {
            // This might happen if we call detach() too fast from different threads.
            return false;
        }
        const NodePtr& child = childIt->second;

        detail::SubTreeLock subTreeLock(child);

        for (auto it = _mountPoints.rbegin(); it != _mountPoints.rend(); ++it)
        {
            it->storageNode->_detachMountedNodeChild(it->depth, name, child);
        }

        _children.erase(childIt);

        return true;
    }

    void Node::_onMounting(StorageNode* storageNode, size_t depth, uint32_t priority)
    {
        _mountPoints.emplace_back(storageNode, depth, priority);
    }

    void Node::_onUnmounted(StorageNode* storageNode, size_t depth)
    {
        auto it = std::find_if(_mountPoints.rbegin(), _mountPoints.rend(), [&](const MountPoint& mountPoint)
        {
            return mountPoint.storageNode == storageNode && mountPoint.depth == depth;
        });

        if (it == _mountPoints.rend())
        {
            assert(false);
            return;
        }

        _mountPoints.erase(std::next(it).base());
    }

} // namespace jbkvs
