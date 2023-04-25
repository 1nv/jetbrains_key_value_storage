#pragma once

#include <atomic>

namespace jbkvs::detail
{

    class SpinLock
    {
        std::atomic<bool> _locked = false;
    public:
        void lock() noexcept
        {
            while (true)
            {
                if (!_locked.exchange(true, std::memory_order_acquire))
                {
                    return;
                }
                while (_locked.load(std::memory_order_relaxed))
                {
                    _mm_pause();
                }
            }
        }

        bool try_lock() noexcept
        {
            return !_locked.load(std::memory_order_relaxed) && !_locked.exchange(true, std::memory_order_acquire);
        }

        void unlock() noexcept
        {
            _locked.store(false, std::memory_order_release);
        }
    };

} // namespace jbkvs::detail
