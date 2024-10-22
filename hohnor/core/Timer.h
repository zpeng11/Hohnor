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
    class Timer : NonCopyable
    {
        /*
        * Should be a noncopyable object that created and managed by eventloop/timerqueue 
        */
    friend struct TimerQueue;
    private:
        const TimerCallback callback_;
        Timestamp expiration_;
        //if greater than 0 means it is a repeated event
        double interval_;
        const int64_t sequence_;
        bool disabled_;

        static std::atomic<uint64_t> s_numCreated_;

        void run(Timestamps, std::weak_ptr<Timer>);
        void restart(Timestamp now);
        

    public:
        Timer(TimerCallback callback, Timestamp when, double interval);

        void disable();

        Timestamp expiration() const { return expiration_; }
        inline bool repeat() const { return interval_ > 0.0; }
        inline bool repeatInterval() const { return interval_; }

        int64_t sequence() const { return sequence_; }
        static int64_t numCreated() { return s_numCreated_; }

        ~Timer() = default;
    };

    //TODO make a TimerHandle class, which holds a weak_ptr to Timer, update TimerQueue to only use disable to mange ending of a timer.
} // namespace Hohnor
