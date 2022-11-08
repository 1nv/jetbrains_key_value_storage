#pragma once

#include <stdint.h>
#include <variant>
#include <string>
#include <memory>

#include <jbkvs/detail/concurrentMap.h>
#include <jbkvs/types/blob.h>

namespace jbkvs
{

    using NodePtr = std::shared_ptr<class Node>;
    using NodeWeakPtr = std::weak_ptr<class Node>;
    using TKey = uint32_t;

    class Node
        : public detail::NonCopyableMixin<Node>
    {
        friend class StorageNode;

        using TValue = std::variant<uint32_t, uint64_t, float, double, std::string, types::BlobPtr>;

        NodeWeakPtr _parent;
        const std::string _name;
        std::shared_mutex _mountMutex;
        size_t _mountCounter;
        detail::ConcurrentMap<std::string, NodePtr> _children;
        detail::ConcurrentMap<TKey, TValue> _data;

    public:
        static NodePtr create();
        static NodePtr create(const NodePtr& parent, const std::string& name);

        bool detach();

        NodePtr getChild(const std::string& name) const;

        template <typename T>
        std::optional<T> get(const TKey& key) const
        {
            std::optional<TValue> value = _data.get(key);
            if (!value)
            {
                return {};
            }

            T* data = std::get_if<T>(&*value);
            return data ? std::optional<T>(*data) : std::optional<T>();
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

        const detail::ConcurrentMap<std::string, NodePtr>& getChildren() const
        {
            return _children;
        }

    private:
        Node(const NodePtr& parent, const std::string& name);
        ~Node();

        void _onMounting();
        void _onUnmounted();
    };

} // namespace jbkvs
