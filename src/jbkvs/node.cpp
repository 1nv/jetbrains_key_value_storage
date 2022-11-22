#include <jbkvs/node.h>

#include <assert.h>

namespace jbkvs
{

    NodePtr Node::create()
    {
        return create({}, {});
    }

    NodePtr Node::create(const NodePtr& parent, const std::string_view& name)
    {
        struct MakeSharedEnabledNode : public Node
        {
            MakeSharedEnabledNode(const NodePtr& parent, std::string&& name)
                : Node(parent, std::move(name))
            {
            }
        };

        if (parent)
        {
            std::shared_lock lock(parent->_mountMutex);

            if (parent->_mountCounter > 0)
            {
                return NodePtr();
            }

            NodePtr newNode = std::make_shared<MakeSharedEnabledNode>(parent, std::string(name));
            parent->_children.put(newNode->_name, newNode);
            return newNode;
        }
        else
        {
            NodePtr newNode = std::make_shared<MakeSharedEnabledNode>(parent, std::string(name));
            return newNode;
        }
    }

    Node::Node(const NodePtr& parent, std::string&& name)
        : _parent(parent)
        , _name(std::move(name))
        , _mountMutex()
        , _mountCounter()
        , _children()
        , _data()
    {
    }

    Node::~Node()
    {
        _children.clear();
    }

    bool Node::detach()
    {
        NodePtr parent = _parent.lock();
        if (parent)
        {
            std::shared_lock lock(parent->_mountMutex);

            if (parent->_mountCounter > 0)
            {
                return false;
            }

            _parent.reset();
            parent->_children.remove(_name);
            return true;
        }
        else
        {
            return false;
        }
    }

    NodePtr Node::getChild(const std::string_view& name) const
    {
        std::optional<NodePtr> optionalPtr = _children.get(name);
        return optionalPtr ? *optionalPtr : NodePtr();
    }

    void Node::_onMounting()
    {
        std::unique_lock lock(_mountMutex);

        ++_mountCounter;
    }

    void Node::_onUnmounted()
    {
        std::unique_lock lock(_mountMutex);

        assert(_mountCounter > 0);
        --_mountCounter;
    }

} // namespace jbkvs
