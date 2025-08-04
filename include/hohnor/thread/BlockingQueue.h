/**
 * Blocking queue that works as normal queue but it is thread safe.
 * It also has infinit head-tail distance that you can put as many item as wish
 * But carrying data type should be copyable and has its default constructor
 */
#pragma once

#include "Mutex.h"
#include "Condition.h"
#include "hohnor/thread/SyncQueue.h"
#include <deque>
#include <atomic>

namespace Hohnor
{
    template <typename T>
    class BlockingQueue : public SyncQueue<T>
    {
    public:
        BlockingQueue()
            : mutex_(),
              notEmpty_(mutex_),
              queue_(),
              end_(false)
        {
        }

        void put(const T &x)
        {
            MutexGuard lock(mutex_);
            queue_.push_back(x);
            notEmpty_.notify(); // wait morphing saves us
                                // http://www.domaigne.com/blog/computing/condvars-signal-with-mutex-locked-or-not/
        }

        void put(T &&x)
        {
            MutexGuard lock(mutex_);
            queue_.push_back(std::move(x));
            notEmpty_.notify();
        }

        T take()
        {
            MutexGuard lock(mutex_);
            // always use a while-loop, due to spurious wakeup
            while (queue_.empty() && !end_)
            {
                notEmpty_.wait();
            }
            if (UNLIKELY(end_))
            {
                return T();
            }
            assert(!queue_.empty());
            T front(std::move(queue_.front()));
            queue_.pop_front();
            return std::move(front);
        }

        size_t size() const
        {
            MutexGuard lock(mutex_);
            return queue_.size();
        }

        //Give up all contains inside the queue, and disable the functionality of putting and taking
        //The blocking take() will return defualt contructed T object.
        //(please do not use the object after calling)
        void giveUp()
        {
            MutexGuard guard(mutex_);
            end_ = true;
            notEmpty_.notifyAll();
        }

    private:
        mutable Mutex mutex_;
        Condition notEmpty_;
        std::deque<T> queue_;
        std::atomic<bool> end_;
    };

}