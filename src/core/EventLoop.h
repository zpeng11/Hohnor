/**
 * The core of the Hohnor library manages events in loop,
 * which is simular to a IO event dispatcher
 */
#pragma once
#include "NonCopyable.h"
#include "CurrentThread.h"
#include "Callbacks.h"
#include "BlockingQueue.h"
#include "Epoll.h"
#include "BinaryHeap.h"
#include <atomic>
namespace Hohnor
{

    class Event;
    class SignalEvent;
    class TimerEventQueue;
    class EventLoop : NonCopyable
    {
    public:
        void loop();
        void setThreadPools(size_t size);
        //Put callbacks in the pendingFunctors
        void runInLoop(Functor callback, int priority = 0);
        void runInPool(Functor callback, int priority = 0);
        void wakeUp();
        void endLoop();

    private:
        //The essential of eventloop
        Epoll poller_;
        std::atomic<bool> quit_;
        const pid_t threadId_;
        uint64_t iteration_; //for debug
        std::unique_ptr<Event> wakeUpEvent_;
        std::unique_ptr<SignalEvent> signalEvent_;
        std::unique_ptr<TimerEventQueue> timers_;

        std::atomic<bool> runningFunctors_;
        //used for protecting pending Functors in mult-threading
        Mutex pendingFunctorsLock_;
        //priorty map to functor
        std::map<int, Functor> pendingFunctors_;
    };
} // namespace Hohnor
