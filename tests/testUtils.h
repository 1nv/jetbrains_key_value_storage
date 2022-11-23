#pragma once

#include <immintrin.h>

class SimpleLatch
{
    std::atomic<std::ptrdiff_t> _counter;

public:
    explicit SimpleLatch(std::ptrdiff_t counter)
        : _counter(counter)
    {
    }

    void arrive_and_wait()
    {
        --_counter;
        while (_counter.load(std::memory_order_relaxed))
        {
            _mm_pause();
        }
    }
};
