/**
 * The class that takes timers and manage them using Binary heap, its contain should only be acessed in the Loop thread
 */

#pragma once
#include "hohnor/common/NonCopyable.h"
#include "hohnor/time/Timestamp.h"
#include "hohnor/common/Callbacks.h"
#include "hohnor/common/BinaryHeap.h"
#include "hohnor/core/IOHandler.h"
#include "hohnor/io/FdUtils.h"
#include <set>
#include <unordered_map>
#include <vector>

namespace Hohnor
{
    class EventLoop;
    class Timer;
    class TimerHandle;
    struct TimerQueue
    {
        /*
        This struct's method should only be called BY eventloop, IN eventloop thread, which means be queueInLoop()
        */
        explicit TimerQueue(EventLoop * loop);
        ~TimerQueue();

        ///
        /// Schedules the callback to be run at given time,
        /// repeats if @c interval > 0.0.
        ///
        /// Happens in queuInLoop()
        std::weak_ptr<Timer> addTimer(TimerCallback cb,
                         Timestamp when,
                         double interval);

        //For cancellation we do not directly discard the timer object in the heap, but set the running Functor of the specific timer to nullptr
        void cancel(std::weak_ptr<Timer>);
        
        void handleRead();
        FdGuard timerFd_;
        EventLoop * loop_;
        std::unordered_map<int64_t, std::shared_ptr<Timer>> timers_; 
        BinaryHeap<std::pair<Timestamp, std::weak_ptr<Timer>>> heap_;
    };

} // namespace Hohnor
