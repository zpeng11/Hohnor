/**
 * Timer events, this event is not created by the user, it should be managed by a Timerqueue
 */

#pragma once
#include "Timestamp.h"
#include "NonCopyable.h"
#include "Copyable.h"
#include "Callbacks.h"
#include <atomic>

namespace Hohnor
{
    class TimerId;
    class Timer : NonCopyable
    {
    private:
        const TimerCallback callback_;
        Timestamp expiration_;
        //if greater than 0 means it is a repeated event
        double interval_;
        const int64_t sequence_;
        bool disabled_;

        static std::atomic<uint64_t> s_numCreated_;

    public:
        Timer(TimerCallback callback, Timestamp when, double interval);
        void run();
        void disable();
        Timestamp expiration() const { return expiration_; }
        inline bool repeat() const { return interval_ > 0.0; }
        int64_t sequence() const { return sequence_; }
        void restart(Timestamp now);
        static int64_t numCreated() { return s_numCreated_; }
        TimerId id();
        ~Timer() = default;
    };

    class TimerId : public Copyable
    {
    public:
        TimerId()
            : timer_(NULL),
              sequence_(0)
        {
        }

        TimerId(Timer *timer, int64_t seq)
            : timer_(timer),
              sequence_(seq)
        {
        }

        // default copy-ctor, dtor and assignment are okay

        friend class TimerQueue;

    private:
        Timer *timer_;
        int64_t sequence_;
    };
} // namespace Hohnor
