# Design path
## Thread related design
### Create basic tools 
+ `Types.h` memset and upper down type cast
+ `Copyable.h` enable copy function
+ `NonCopyable.h` for classes to inherit so that child classes become uncopyable
+ `Atomic.h` for atomic variables(we now replace with c++11 std::atomic)
+ `CurrentThread.h` for monitoring thread information from thread local level
### Wrap pthread functions into classes
+ `Mutex.h` wrapped pthread mutex
+ `Condition.h` wrapped pthread cond