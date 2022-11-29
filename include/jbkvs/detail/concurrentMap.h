#pragma once

#include <shared_mutex>
#include <map>
#include <optional>

#include <jbkvs/detail/mixins.h>

namespace jbkvs::detail
{

    class SharedMutexMapConstIteratorEndTag
    {
    };

    template <typename TKey, typename TValue>
    class SharedMutexMapConstIterator
        : NonCopyableMixin<SharedMutexMapConstIterator<TKey, TValue>>
    {
        std::shared_lock<std::shared_mutex> _lock;
        typename std::map<TKey, TValue, std::less<>>::const_iterator _it;
        typename std::map<TKey, TValue, std::less<>>::const_iterator _endIt;

    public:
        SharedMutexMapConstIterator(std::shared_mutex& mutex, const std::map<TKey, TValue, std::less<>>& map)
            : _lock(mutex)
            , _it(map.begin())
            , _endIt(map.end())
        {
        }

        ~SharedMutexMapConstIterator()
        {
        }

        const std::pair<const TKey, TValue>& operator*() const noexcept
        {
            return *_it;
        }

        SharedMutexMapConstIterator& operator++() noexcept
        {
            ++_it;
            return *this;
        }

        bool operator!=(const SharedMutexMapConstIteratorEndTag& endTag) const noexcept
        {
            return _it != _endIt;
        }
    };

    template <typename TKey, typename TValue>
    class SharedMutexMap
        : public NonCopyableMixin<SharedMutexMap<TKey, TValue>>
    {
        mutable std::shared_mutex _mutex;
        std::map<TKey, TValue, std::less<>> _map;
    public:
        SharedMutexMap() noexcept
            : _mutex()
            , _map()
        {
        }

        ~SharedMutexMap()
        {
        }

        template <typename TCustomKey>
        std::optional<TValue> get(TCustomKey&& key) const
        {
            std::shared_lock lock(_mutex);

            auto it = _map.find(std::forward<TCustomKey>(key));
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

        SharedMutexMapConstIterator<TKey, TValue> begin() const
        {
            return SharedMutexMapConstIterator<TKey, TValue>(_mutex, _map);
        }

        SharedMutexMapConstIteratorEndTag end() const
        {
            return {};
        }
    };

    // TODO: provide lock-free map implementation.

    template <typename TKey, typename TValue>
    using ConcurrentMap = SharedMutexMap<TKey, TValue>;

} // namespace jbkvs::detail
