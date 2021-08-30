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
using namespace std;

Hohnor::ThreadPool tp;
Hohnor::Mutex m;
Hohnor::AsyncLog al("asyncLog");

using namespace Hohnor;

int main()
{
    Hohnor::g_logLevel = Hohnor::Logger::TRACE;
    Hohnor::Logger::setAsyncLog(&al);
    auto func = [&]()
    {
        LOG_DEBUG << "Enter thread";
    };
    tp.start(5);
    for (int i = 0; i < 20000; i++)
    {
        tp.run(func);
    }
    sleep(1);
}