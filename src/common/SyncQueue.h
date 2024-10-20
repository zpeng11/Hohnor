/**
 * Common interface for syncnized queue, implementations can be bounded or unbounded
 */
#pragma once
#include "NonCopyable.h"

namespace Hohnor
{
    /**
     * Interface class of synchronized queues, can be implemented as bounded or unbounded
     */
    template <typename T>
    class SyncQueue
    {
    public:
        SyncQueue()= default;
        ~SyncQueue() = default;
        virtual void put(const T &x) = 0;
        virtual void put(T &&x) = 0;
        virtual T take() = 0;
        virtual size_t size() const = 0;
        virtual void giveUp() = 0;
    };
} // namespace Hohnor
