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
#include <set>
namespace Hohnor
{

    class Event;
    class IOHandler;
    class SignalHandler;
    class SignalEventSet;
    class TimerQueue;
    class TimerEvent;
    class EventLoop : NonCopyable
    {
    public:
        void loop();
        void setThreadPools(size_t size);
        //Put callbacks in the pendingFunctors
        void runInLoop(Functor callback);
        //Put the callback into threadpool to run
        void runInPool(Functor callback);
        //Ask the loop to wake up from epoll immediately to deal with pending functors
        void wakeUp();
        //End the loop
        void endLoop();

        //Assert that a thread that calls this method is the same thread as the loop
        void assertInLoopThread();

        //Add a signal event to the reactor
        void addSignalEvent(SignalHandler *event);
        //remove a signal event from the reactor
        void removeSignalEvent(SignalHandler *event);
        //if has event
        bool hasSignalEvent(SignalHandler * event);

        //Add a IO event to the epoll
        void addIOEvent(IOHandler * event);
        //modify existing IO event in the epoll
        void updateIOEvent(IOHandler *event);
        //Remove a IO event from the epoll
        void removeIOEvent(IOHandler *event);
        //if has the event
        bool hasIOEvent(IOHandler * event);

        //Add timer event to the event set
        void addTimerEvent(TimerEvent * event);



    private:

        //The essential of eventloop
        Epoll poller_;
        std::atomic<bool> quit_;
        const pid_t threadId_;
        //for debug
        uint64_t iteration_; 

        //For epoll to check if the IO event encountered still exist in the reactor
        std::set<IOHandler *> IOEvents_;

        //Real time wakeup pipe, wakeup the loop from epoll to deal with pending Functors
        std::unique_ptr<IOHandler> wakeUpPipeEvent_;
        //When signal comes, this pipe will get input event, fd life cycle is managed by IO/SignalUtils
        std::unique_ptr<IOHandler> signalPipeEvent_;

        //manage fd life cycle
        FdGuard wakeUpPipeReadEnd_;
        //manage fd life cycle
        FdGuard wakeUpPipeWriteEnd_;

        std::unique_ptr<TimerQueue> timers_;

        std::atomic<bool> runningFunctors_;
        //used for protecting pending Functors in mult-threading, change of evenloop data
        //from other threads are only allowed to commit their change into pendingFunctors,
        //And let the loop thread to actually run these changes, In this way we only need to
        //mantain pendingFunctors to be thread-safe
        Mutex pendingFunctorsLock_;
        //priorty map to functor
        std::deque<Functor> pendingFunctors_;
    };
} // namespace Hohnor
