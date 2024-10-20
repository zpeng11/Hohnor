/**
 * Timer events, this event is not created by the user, it should be managed by a Timerqueue
 */

#pragma once
#include "hohnor/time/Timestamp.h"
#include "hohnor/common/NonCopyable.h"
#include "hohnor/common/Copyable.h"
#include "hohnor/common/Callbacks.h"
#include <atomic>

namespace Hohnor
{
    class TimerHandle;
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
        void run(Timestamp, TimerHandle);
        void disable();
        Timestamp expiration() const { return expiration_; }
        inline bool repeat() const { return interval_ > 0.0; }
        int64_t sequence() const { return sequence_; }
        void restart(Timestamp now);
        static int64_t numCreated() { return s_numCreated_; }
        ~Timer() = default;
    };

    class EventLoop;
    class TimerHandle : public Copyable
    {
    public:
        TimerHandle()
            : timer_(NULL), loop_(NULL)
        {
        }

        TimerHandle(Timer *timer, EventLoop *loop)
            : timer_(timer), loop_(loop) {}

        EventLoop *loop() { return loop_; }

        void cancel();

        // default copy-ctor, dtor and assignment are okay

        friend class EventLoop;

    private:
        Timer *timer_;
        EventLoop *loop_;
    };
} // namespace Hohnor
