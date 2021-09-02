# Design path
## Comoon tools
+ `StringPiece.h` a string class that does not care about life cycle of char byte arrary "Copy-on-write" string in other word, which has fastest speed. see `string_view` in c++14
+ `Types.h` memset and upper down type cast
+ `NonCopyable.h` for classes to inherit so that child classes become uncopyable, also see `Copyable`
## Thread related design
 
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


## Time and Date
+ `Date.h` use Julian date time calculation algorithm to record dates
+ `Timestamp` wrapper for C style time_t, in the unit of microsecond since epoch
+ `TimeZone` it is too hard! I am not sure if this is necesarry

## Logging
+ `LogStream` simulated output stream, use macros to do dynamaic checking and logging infomation
(refer interface from glog)
+ `logging` provide lower level support for LogStream
+ `AsyncLogging` use `FileUtils` and `LogFile` to async thread safe log.

## File
+ `FileUtils` provide utilities of file io for 1.read  2.append
+ `LogFile` use append utility to write log file

## Net
+ `InetAddress` wrapper for linux net address object