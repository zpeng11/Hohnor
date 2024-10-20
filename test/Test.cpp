#include <iostream>
#include <stdio.h>
#include "hohnor/thread/Thread.h"
#include "hohnor/time/Timestamp.h"
#include "hohnor/time/Date.h"
#include "hohnor/log/Logging.h"
#include "hohnor/file/FileUtils.h"
#include "hohnor/process/ProcessInfo.h"
#include "hohnor/thread/ThreadPool.h"
#include <memory>
#include "hohnor/file/LogFile.h"
#include "hohnor/log/AsyncLogging.h"
#include "hohnor/net/SocketWrap.h"
#include "hohnor/net/InetAddress.h"
#include "hohnor/net/Socket.h"
#include "hohnor/process/ProcessInfo.h"
#include "hohnor/net/TCPSocket.h"
#include "hohnor/common/BinaryHeap.h"
#include "hohnor/core/EventLoop.h"
#include "hohnor/core/IOHandler.h"
#include "hohnor/core/SignalSet.h"
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
    auto func = [&i](int signal, SignalHandle handle)
    {
        LOG_INFO << "SIGINT "<<i<<"th time";
        handle.cancel();
    };
    loop.addSignal(SIGINT, func);
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