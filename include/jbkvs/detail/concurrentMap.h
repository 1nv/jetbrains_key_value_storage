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

        ~SharedMutexMap() noexcept
        {
        }

        std::optional<TValue> get(const TKey& key) const noexcept
        {
            std::shared_lock lock(_mutex);

            auto it = _map.find(key);
            return it != _map.end() ? it->second : std::optional<TValue>();
        }

        void put(const TKey& key, const TValue& value) noexcept
        {
            std::unique_lock lock(_mutex);

            _map[key] = value;
        }

        void put(const TKey& key, TValue&& value) noexcept
        {
            std::unique_lock lock(_mutex);

            _map[key] = std::move(value);
        }

        bool remove(const TKey& key) noexcept
        {
            std::unique_lock lock(_mutex);

            bool result = !!_map.erase(key);
            return result;
        }

        void clear() noexcept
        {
            std::unique_lock lock(_mutex);

            _map.clear();
        }

        size_t size() const noexcept
        {
            std::shared_lock lock(_mutex);

            return _map.size();
        }
    };

    // TODO: provide lock-free map implementation.

    template <typename TKey, typename TValue>
    using ConcurrentMap = SharedMutexMap<TKey, TValue>;

} // namespace jbkvs::detail
