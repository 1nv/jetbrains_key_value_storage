#include <jbkvs/node.h>

namespace jbkvs
{

    NodePtr Node::create()
    {
        return create({}, {});
    }

    NodePtr Node::create(const NodePtr& parent, const std::string& subPath)
    {
        struct MakeSharedEnabledNode : public Node {};

        NodePtr newNode = std::make_shared<MakeSharedEnabledNode>();
        if (parent)
        {
            parent->_children.put(subPath, newNode);
        }
        return newNode;
    }

    Node::Node()
        : _children()
        , _data()
    {
    }

    Node::~Node()
    {
        _children.clear();
    }

    NodePtr Node::getChild(const std::string& name) const
    {
        std::optional<NodePtr> optionalPtr = _children.get(name);
        return optionalPtr ? *optionalPtr : NodePtr();
    }

} // namespace jbkvs
