# Design path
## Thread related design
Please Read them by the sequence provided here. If there is any concept that you have no idea about, please do research 
### Create basic tools 
+ `Types.h` memset and upper down type cast
+ `Copyable.h` enable copy function
+ `NonCopyable.h` for classes to inherit so that child classes become uncopyable, also see `Copyable`
+ `Atomic.h` for atomic variables(we now replace with c++11 std::atomic, leave here just to explain basic implementation of atomic)
+ `CurrentThread.h` for monitoring thread information from thread local level
### Wrap pthread functions into classes
+ `Mutex.h` wrapped pthread mutex
+ `Condition.h` wrapped pthread cond
+ `CountDownLatch.h` a multithread countdown latch that uses Condition mechanism, but only for synchronize and count.
+ `Thread.h` a actual thread implementation
### Thread safe tools
+ `BlockingQueue.h` use mutex to rebuild deque that is thread safe.
+ `CircularBuffer.h` prepare for BoundedBlockingQueue
+ `BoundedBlockingQueue.h` same as last one but limited in size, prepared for thread pool
### Threadpool