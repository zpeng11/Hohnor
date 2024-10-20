# Design path

## Modules and dependancies
Will be a good practice to read from top to bottom if someone whats to get into this project
| Name | Functionality | Dependencies |
| -------- | ------- |  ------- |
| common | Common headers, type defines, and utilities | None |
| time | Time and date utils | None |
| thread | Wrapping of pthread, provide utils and a completed threadpool | None |
| file | File utils that employ low level file access and provide service to logging |time|
| log | Provide thread safe logging utils that runs in a seperate thread and collect logs | time, thread, file |
| process | Wrapping of unix process, provide utils and info message | time, file, thread |
|io| Wrapping of FD utils, signal handle, and epoll core| log |
|core | Core eventloop and event utils | io |
|net | TCP and socket support | io |

## Traditional Use Case
1. Use as a reactive framework for interactive commandline software by providing callbacks to timers and keyboard.
2. Use as a reactive TCP server by providing socket callbacks.


## Time and date design
+ `Date` use Julian date time calculation algorithm to record dates
+ `Timestamp` wrapper for C style time_t, in the unit of microsecond since epoch
+ TimeZone it is too hard! I am not sure if this is necesarry

## Thread related design
I know std::thread is good enough, but implementing threadpool and thread-utils from pthread is still meaningful to me :)
### Create basic tools 
+ `Atomic.h` for atomic variables(we now replace with c++11 std::atomic, leave here just to explain basic implementation of atomic)
+ `CurrentThread.h` for monitoring thread information from thread local level
+ `Exception.h` exception class for multi-thread env
+ `Singleton.h` Thread level Singleton object template class
### Wrap pthread functions into classes
+ `Mutex.h` wrapped pthread mutex
+ `Condition.h` wrapped pthread cond
+ `CountDownLatch.h` a multithread countdown latch that uses Condition mechanism, but only for synchronize and count.
+ `Thread.h` a actual thread implementation
### Thread safe tools
+ `BlockingQueue.h` use mutex to rebuild deque that is thread safe.
+ `CircularBuffer.h` prepare for BoundedBlockingQueue
+ `BoundedBlockingQueue.h` same as blocking queue but limited in size, prepared for thread pool
### Threadpool
+ `ThreadPool.h` implement threadpool

## Logging related design
The logging function works in an async way, which means the earlier executed log code is not garenteed to be in a upper line inside log output, this happens because we only flush string of log when the logger object's destructor stage. The message in log messages has timestamps to ensure reader can understand what actually happened. Logging thread is standalone from all threadpool managed threads(which allows run outside of threadpool lifespan). The content of log are produced and stored in a smart-ptr-managed buffer, then put in thread-safe queue for stdout/filewriter to comsume. 
+ `LogStream` simulated output stream, use macros to do dynamaic checking and logging infomation
(refer interface from glog)
+ `logging` provide lower level support for LogStream
+ `AsyncLogging` use `FileUtils` and `LogFile` to async thread safe log.



## Net
+ `InetAddress` wrapper for linux net address object