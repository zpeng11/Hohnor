#include <iostream>
#include <stdio.h>
#include "Thread.h"
#include "Timestamp.h"
#include "Date.h"
#include "Logging.h"
#include "FileUtils.h"
#include "ProcessInfo.h"
#include "ThreadPool.h"
#include <memory>
#include "LogFile.h"
#include "LogFile.h"
#include "AsyncLogging.h"
#include "SocketWrap.h"
#include "InetAddress.h"
#include "Socket.h"
#include "ProcessInfo.h"
#include "TCPSocket.h"
#include "BinaryHeap.h"
#include "EventLoop.h"
#include "IOHandler.h"
#include "SignalHandlerSet.h"
#include <signal.h>
#include <sys/timerfd.h>
using namespace std;

using namespace Hohnor;

void onConnectionClose(EventLoop *loop, IOHandler *handler);
void onConnectionRead(EventLoop *loop, IOHandler *handler, int fd)
{
    EventLoop *possible = EventLoop::loopOfCurrentThread();
    LOG_INFO << "Read from the client";
    char str[BUFSIZ];
    int ret = ::read(fd, str, BUFSIZ);
    LOG_INFO << "Read return val:" << ret;
    if (ret < 0)
        LOG_SYSERR << "Read error";
    if (ret == 0)
    {
        onConnectionClose(loop, handler);
    }
    if (ret > 0)
        LOG_INFO << str;
}

void onConnectionClose(EventLoop *loop, IOHandler *handler)
{
    LOG_INFO << "Client closed";
    loop->queueInLoop([handler]()
                      { delete handler; });
}

void onAccept(EventLoop *loop, TCPListenSocket *socket)
{
    auto pair = socket->accept();
    int fd = pair.fd();
    IOHandler *connectionHandler = new IOHandler(loop, pair.fd());
    connectionHandler->setReadCallback([loop, connectionHandler, fd]()
                                       { onConnectionRead(loop, connectionHandler, fd); });
    connectionHandler->setCloseCallback([loop, connectionHandler]()
                                        { onConnectionClose(loop, connectionHandler); });
    connectionHandler->enable();
}

int main(int argc, char *argv[])
{
    Logger::setGlobalLogLevel(Logger::LogLevel::INFO);
    EventLoop loop;
    TCPListenSocket socket;
    socket.bindAddress(9911);
    socket.listen();
    IOHandler serverHandler(&loop, socket.fd());
    serverHandler.setReadCallback([&loop, &socket]()
                                  { onAccept(&loop, &socket); });
    serverHandler.enable();
    int j = 0;
    loop.addTimer([&j](Timestamp now, TimerHandle handle)
                  { LOG_INFO << "Timer up:"<<now.toFormattedString();
                  j = j+1;
                  if(j>3)
                  {
                      LOG_INFO << "Timer finished:";
                      handle.cancel();
                  } }
                  ,addTime(Timestamp::now(), 1), 1);
    int i = 0;
    SignalHandlerId sigId;
    auto func = [&i, &sigId, &loop](int signal)
    {
        LOG_INFO << "SIGINT "<<i<<"th time";
            loop.removeSignalHandler(sigId);
    };
    sigId = loop.addSignalHandler(SIGINT, func);
    loop.loop();

    // int fd = ::timerfd_create(CLOCK_MONOTONIC,
    //                           TFD_NONBLOCK | TFD_CLOEXEC);
    // struct itimerspec new_value;
    // memZero(&new_value, sizeof(struct itimerspec));
    // new_value.it_value.tv_sec = 2; //100ms
    // LOG_INFO << "Start";
    // int ret = ::timerfd_settime(fd, 0, &new_value, NULL);
    // if (ret < 0)
    //     LOG_SYSFATAL;
    // uint64_t exp;
    // while (read(fd, &exp, sizeof(uint64_t)) != sizeof(uint64_t))
    // {
    // }
    // LOG_INFO << Timestamp::now().toFormattedString();
}