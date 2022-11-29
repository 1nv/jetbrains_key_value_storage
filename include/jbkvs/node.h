#pragma once

#include <stdint.h>
#include <variant>
#include <string_view>
#include <memory>
#include <vector>

#include <jbkvs/detail/concurrentMap.h>
#include <jbkvs/types/blob.h>

namespace jbkvs
{

    using NodePtr = std::shared_ptr<class Node>;
    using NodeWeakPtr = std::weak_ptr<class Node>;
    using TKey = uint32_t;

    class StorageNode;

    namespace detail
    {

        class SubTreeLock
        {
            NodePtr _node;
        public:
            explicit SubTreeLock(const NodePtr& node);
            ~SubTreeLock();
        };

    } // namespace detail

    class Node
        : public detail::NonCopyableMixin<Node>
    {
        friend class StorageNode;
        friend class detail::SubTreeLock;

        using TValue = std::variant<uint32_t, uint64_t, float, double, std::string, types::BlobPtr>;

        struct MountPoint
        {
            StorageNode* storageNode;
            size_t depth;
            uint32_t priority;

            MountPoint(StorageNode* storageNode, size_t depth, uint32_t priority) : storageNode(storageNode), depth(depth), priority(priority) {}
        };

        NodeWeakPtr _parent;
        const std::string _name;
        mutable std::shared_mutex _mutex;
        std::vector<MountPoint> _mountPoints;
        std::map<std::string, NodePtr, std::less<>> _children;
        detail::ConcurrentMap<TKey, TValue> _data;

    public:
        static NodePtr create();
        static NodePtr create(const NodePtr& parent, const std::string_view& name);

        bool detach();

        const std::string& getName() const { return _name; }

        NodePtr getParent() const;
        NodePtr getChild(const std::string_view& name) const;

        template <typename T>
        std::optional<T> get(const TKey& key) const
        {
            std::optional<TValue> value = _data.get(key);
            if (!value)
            {
                return {};
            }

            T* data = std::get_if<T>(&*value);
            return data ? std::optional<T>(std::move(*data)) : std::optional<T>();
        }

        template <typename T>
        void put(const TKey& key, T&& value)
        {
            _data.put(key, std::forward<T>(value));
        }

        bool remove(const TKey& key)
        {
            return _data.remove(key);
        }

        class ChildrenMapWrapper
        {
            // TODO: think if it is better to hold NodePtr here (requires shared_from_this, decreases performance).
            // Currently the object can only be operated while someone holds a NodePtr towards target node.
            std::shared_mutex& _mutex;
            const std::map<std::string, NodePtr, std::less<>>& _children;

        public:
            ChildrenMapWrapper(std::shared_mutex& mutex, const std::map<std::string, NodePtr, std::less<>>& children) noexcept
                : _mutex(mutex)
                , _children(children)
            {
            }

            NodePtr get(const std::string_view& name) const
            {
                std::shared_lock lock(_mutex);

                auto it = _children.find(name);
                return (it != _children.end()) ? it->second : NodePtr();
            }

            size_t size() const
            {
                std::shared_lock lock(_mutex);

                return _children.size();
            }

            detail::SharedMutexMapConstIterator<std::string, NodePtr> begin() const
            {
                return detail::SharedMutexMapConstIterator<std::string, NodePtr>(_mutex, _children);
            }

            detail::SharedMutexMapConstIteratorEndTag end() const
            {
                return {};
            }
        };

        ChildrenMapWrapper getChildren() const noexcept
        {
            return ChildrenMapWrapper(_mutex, _children);
        }

    private:
        Node(const NodePtr& parent, const std::string_view& name);
        ~Node();

        void _lockSubTree();
        void _unlockSubTree();

        bool _attachChild(const std::string& name, const NodePtr& child);
        bool _detachChild(const std::string& name);

        void _onMounting(StorageNode* storageNode, size_t depth, uint32_t priority);
        void _onUnmounted(StorageNode* storageNode, size_t depth);
    };

} // namespace jbkvs
