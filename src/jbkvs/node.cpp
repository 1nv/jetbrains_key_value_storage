#include <jbkvs/node.h>

namespace jbkvs
{

    NodePtr Node::create()
    {
        return create({}, {});
    }

    NodePtr Node::create(const NodePtr& parent, const std::string& name)
    {
        struct MakeSharedEnabledNode : public Node
        {
            MakeSharedEnabledNode(const NodePtr& parent, const std::string& name)
                : Node(parent, name)
            {
            }
        };

        NodePtr newNode = std::make_shared<MakeSharedEnabledNode>(parent, name);
        if (parent)
        {
            parent->_children.put(name, newNode);
        }
        return newNode;
    }

    Node::Node(const NodePtr& parent, const std::string& name)
        : _parent(parent)
        , _name(name)
        , _children()
        , _data()
    {
    }

    Node::~Node()
    {
        _children.clear();
    }

    void Node::detach()
    {
        NodePtr parent = _parent.lock();
        if (parent)
        {
            _parent.reset();
            parent->_children.remove(_name);
        }
    }

    NodePtr Node::getChild(const std::string& name) const
    {
        std::optional<NodePtr> optionalPtr = _children.get(name);
        return optionalPtr ? *optionalPtr : NodePtr();
    }

} // namespace jbkvs
