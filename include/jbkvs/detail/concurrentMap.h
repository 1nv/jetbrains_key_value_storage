#pragma once

#include <shared_mutex>
#include <map>
#include <optional>

#include <jbkvs/detail/mixins.h>

namespace jbkvs::detail
{

    template <typename TKey, typename TValue>
    class SharedMutexMap
        : public NonCopyableMixin<SharedMutexMap<TKey, TValue>>
    {
        mutable std::shared_mutex _mutex;
        std::map<TKey, TValue> _map;
    public:
        SharedMutexMap() noexcept
            : _mutex()
            , _map()
        {
        }

        ~SharedMutexMap()
        {
        }

        std::optional<TValue> get(const TKey& key) const
        {
            std::shared_lock lock(_mutex);

            auto it = _map.find(key);
            return it != _map.end() ? it->second : std::optional<TValue>();
        }

        void put(const TKey& key, const TValue& value)
        {
            std::unique_lock lock(_mutex);

            _map[key] = value;
        }

        void put(const TKey& key, TValue&& value)
        {
            std::unique_lock lock(_mutex);

            _map[key] = std::move(value);
        }

        bool remove(const TKey& key)
        {
            std::unique_lock lock(_mutex);

            bool result = !!_map.erase(key);
            return result;
        }

        void clear()
        {
            std::unique_lock lock(_mutex);

            _map.clear();
        }

        size_t size() const
        {
            std::shared_lock lock(_mutex);

            return _map.size();
        }

        class ConstIteratorEndTag
        {
        };

        class ConstIterator
            : NonCopyableMixin<ConstIterator>
        {
            std::shared_lock<std::shared_mutex> _lock;
            typename std::map<TKey, TValue>::const_iterator _it;
            typename std::map<TKey, TValue>::const_iterator _endIt;

        public:
            ConstIterator(const SharedMutexMap<TKey, TValue>& map)
                : _lock(map._mutex)
                , _it(map._map.begin())
                , _endIt(map._map.end())
            {
            }

            ~ConstIterator()
            {
            }

            const std::pair<const TKey, TValue>& operator*() const noexcept
            {
                return *_it;
            }

            ConstIterator& operator++() noexcept
            {
                ++_it;
                return *this;
            }

            bool operator!=(const ConstIteratorEndTag& endTag) const noexcept
            {
                return _it != _endIt;
            }
        };

        ConstIterator begin() const
        {
            return ConstIterator(*this);
        }

        ConstIteratorEndTag end() const
        {
            return ConstIteratorEndTag();
        }
    };

    // TODO: provide lock-free map implementation.

    template <typename TKey, typename TValue>
    using ConcurrentMap = SharedMutexMap<TKey, TValue>;

} // namespace jbkvs::detail
