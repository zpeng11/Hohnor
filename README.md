# Hohnor
This is an Unix-only library that provides event-driven asynchronous IO and Network management base on Reactive IO model written in C++.

# Read from here
hohnor/common/*
hohnor/thread/CurrentThread
hohnor/thread/Mutex
hohnor/thread/Condition
hohnor/thread/BlockingQueues
hohnor/thread/CountDownLatch
hohnor/thread/Thread
hohnor/thread/ThreadPool

# TODO List:
* Use signalfd to handle signal
* Add support for keyboard input event
* Intruduce GTest and makes unit test cases
* Create alternative for epoll on cygwin(poll)
* Create cmake interface and doc instruction for other project to import
* Wrapup documentations
* Create machanism to restrictly check and prevent user fork happens after evenloop happens(fd safety)
* Log enable message with defferent level to have differnt way of write and flush
* Current thread id management is highly depend on linux platform's pid system instead of pure pthread.