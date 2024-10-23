/**
 * Timer events, this event is not created by the user, it should be managed by a Timerqueue
 */

#pragma once
#include "hohnor/time/Timestamp.h"
#include "hohnor/common/NonCopyable.h"
#include "hohnor/common/Copyable.h"
#include "hohnor/common/Callbacks.h"
#include "hohnor/log/Logging.h"
#include <atomic>

namespace Hohnor
{
    class Timer : NonCopyable
    {
        /*
        * Should be a noncopyable object that created and managed by eventloop/timerqueue 
        * because all the updates must happen in loop thread.
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

        void run(TimerHandle);
        void restart(Timestamp now);
        

    public:
        Timer(TimerCallback callback, Timestamp when, double interval);

        void disable();

        Timestamp expiration() const { return expiration_; }
        inline bool repeat() const { return interval_ > 0.0; }
        inline double repeatInterval() const { return interval_; }

        int64_t sequence() const { return sequence_; }
        static int64_t numCreated() { return s_numCreated_; }

        ~Timer() = default;
    };

//TODO, when calling a Eventloop::createTimer() somewhere in other thread, create a shared_ptr of Timer, 
//place the timer obj into queue_ by running in loopthread, then create weak_ptr from the shared and return.
    class TimerHandle: Copyable{
        /*
        * A copyable object that obtained by uses to manage timer(Specifically knowning timer info and disable)
        * This is an object users use to interact with timer.
        */
        friend struct TimerQueue;
        private:
        std::weak_ptr<Timer> timer_;
        EventLoop * loop_;
        TimerQueue * queue_;
        TimerHandle(EventLoop * loop, TimerQueue * queue, std::weak_ptr<Timer> timer):timer_(timer), queue_(queue), loop_(loop){}
        public:
        bool valid() const { return timer_.lock()? true: false; }
        void disable();
        Timestamp expiration() const { if(!valid()) LOG_ERROR<<"This timer has been expired or disabled"; return timer_.lock()->expiration(); }
        inline bool repeat() const { if(!valid()) LOG_ERROR<<"This timer has been expired or disabled"; return timer_.lock()->repeat(); }
        inline double repeatInterval() const { if(!valid()) LOG_ERROR<<"This timer has been expired or disabled"; return timer_.lock()->repeatInterval(); }
    };

    //TODO make a TimerHandle class, which holds a weak_ptr to Timer, update TimerQueue to only use disable to mange ending of a timer.
} // namespace Hohnor
