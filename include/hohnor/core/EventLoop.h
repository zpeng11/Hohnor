/**
 * The core of the Hohnor library manages events in loop,
 * which is simular to a IO event dispatcher
 */


#pragma once
#include "hohnor/common/NonCopyable.h"
#include "hohnor/common/Callbacks.h"
#include "hohnor/time/Timestamp.h"
#include "Signal.h"

#include <atomic>
#include <vector>
#include <set>
#include <unordered_map>
#include <memory>
namespace Hohnor
{

    class Event;
    class IOHandler;
    class TimerQueue;
    class TimerHandler;
    class Timestamp;
    class SignalHandler;
    class Mutex;
    class Epoll;
    class ThreadPool;
    /**
     * 
     */
    class EventLoop : public NonCopyable, public std::enable_shared_from_this<EventLoop>
    {
        friend class IOHandler;
    private:
        EventLoop();
    public:
        static std::shared_ptr<EventLoop> createEventLoop();
        ~EventLoop();

        static EventLoop *loopOfCurrentThread();

        void setThreadPools(size_t size);
        void loop();

        //If in the loop thread run immediatly, if not queue the callback in pending, thread safe
        void runInLoop(Functor callback);
        //Put callbacks in the pendingFunctors, thread safe
        void queueInLoop(Functor callback);
        //Put the callback into threadpool to run, thread safe
        void runInPool(Functor callback);

        //Ask the loop to wake up from epoll immediately to deal with pending functors
        void wakeUp();
        //End the loop
        void endLoop();
        //Get the iteration num
        int64_t iteration() { return iteration_; }

        //Return timestamp of last poll_wait return
        Timestamp pollReturnTime() { return pollReturnTime_; }

        //Assert that a thread that calls this method is the same thread as the loop
        void assertInLoopThread();

        //handle an IO FD into the loop, and create a handler for it, will take over fd's ownership and lifecycle, threadsafe
        std::shared_ptr<IOHandler> handleIO(int fd);

        //Add timer event, threadsafe
        //If interval > 0, it is a repeated timer
        std::shared_ptr<TimerHandler> addTimer(TimerCallback cb, Timestamp when, double interval = 0.0f);

        //Handle a signal in a way you expect, thread safe
        void handleSignal(int signal, SignalAction action, SignalCallback cb = nullptr);

        //Manage keyboard in an interactive Non-Canonical way, put nullptr to be back to normal, thread safe
        void handleKeyboard(KeyboardCallback cb);

        //There are 3 working phases of a loop process: polling, IO handling, and pending handling
        //After ctor and Before calling loop(), it is Ready state,
        //After calling endLoop(), it is end state
        enum LoopState
        {
            Ready,
            Polling,
            IOHandling,
            PendingHandling,
            End
        };

        //Get the current state of the loop
        LoopState state() const { return state_; }

        //Check if the loop is quitting
        bool isQuited() const { return quit_; }

    private:
        //The essential of eventloop
        Epoll * poller_;

        std::atomic<bool> quit_;
        pid_t threadId_;
        //for debug
        uint64_t iteration_;

        std::atomic<LoopState> state_;

        //Time when epoll returns
        std::atomic<Timestamp> pollReturnTime_;

        //Real time wakeup pipe, wakeup the loop from epoll to deal with pending Functors
        std::shared_ptr<IOHandler> wakeUpHandler_;

        TimerQueue * timers_;

        //used for protecting pending Functors in mult-threading, change of evenloop data
        //from other threads are only allowed to commit their change into pendingFunctors,
        //And let the loop thread to actually run these changes, In this way we only need to
        //mantain pendingFunctors to be thread-safe
        Mutex * pendingFunctorsLock_;
        //priorty map to functor
        std::vector<Functor> pendingFunctors_;

        std::unordered_map<int, std::shared_ptr<SignalHandler>> signalMap_;

        //ThreadPool for running tasks in background threads
        std::unique_ptr<ThreadPool> threadPool_;

        static std::shared_ptr<IOHandler> interactiveIOHandler_;

        //To bind for wake up event
        void handleWakeUp();

        bool isLoopThread();

        //modify or remove existing IO event in the epoll
        void updateIOHandler(std::shared_ptr<IOHandler> handler, bool addNew);

        //Remove a fd from epoll, 
        //used by IOHandler's destructor because 
        //in dtor we can not take out smart ptr to pass into function closure.
        void removeFd(int fd);
    };
} // namespace Hohnor
