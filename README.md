# Hohnor
This is an Unix-only library that provides event-driven asynchronous IO and Network management base on Reactive IO model written in C++. Provide useful toolkits for Linux application developement.

## Utilities Modules
### Time
Linux time and date class and methods abstraction utilities.
### Thread
Linux pthread class and method abstraction utilities, and threadpool definition.
### File
Linux file utilities.
### Process
Linux process info and utilities.
### IO
Linux FD, Epoll, and Signal wrappers and utilities.

## Application Modules
### Log
Logging system works in multithreading environment, supports file/stdout destination, supports multiple logging level, and provides useful logging/assertion macros.
### Core
Core of the project, provide eventloop and controls to IO, Timer, and Signal using handlers.
### Net
Network utilities and TCP implementation.


## Module Relationship
```
Hohnor
 └──core/net
     └──io
         └──process/log
             ├──thread
             └──file
                 └──time
```


# TODO List:
* Add support for keyboard input event
* Create cmake interface and doc instruction for other project to import
* Create machanism to restrictly check and prevent user fork happens after evenloop happens(fd safety)
* Log enable message with defferent level to have differnt way of write and flush
* Github CI