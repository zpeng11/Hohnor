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
using namespace std;

using namespace Hohnor;

int main(int argc, char *argv[])
{
    Logger::setGlobalLogLevel(Logger::LogLevel::TRACE);
    EventLoop loop;
    loop.wakeUp();
    loop.loop();
}