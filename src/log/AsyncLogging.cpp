#include "AsyncLogging.h"
#include <functional>
#include "Types.h"

using namespace Hohnor;

AsyncLog::AsyncLog(const string &basename,
                   const string &directory,
                   int checkEveryN,
                   int flushInterval,
                   off_t rollSize,
                   int rollInterval,
                   Timestamp::TimeStandard standrad)
    : logFile_(basename, directory, checkEveryN, flushInterval, rollSize, rollInterval, standrad),
      logThread_(std::bind(&Hohnor::AsyncLog::logThreadFunc, this), "AsyncLogThread"),
      bufferQueue_(),
      stop_(false)
{
    logThread_.start();
}

AsyncLog::~AsyncLog()
{
    stop_ = true;
    flush();
    logThread_.join();
}

void AsyncLog::logThreadFunc()
{
    while (UNLIKELY(!stop_))
    {
        auto buffer = bufferQueue_.take();
        if (UNLIKELY(buffer == nullptr)) //That is a flush
        {
            logFile_.flush();
        }
        else   //normal data
        {
            auto msg = buffer->data();
            auto len = buffer->length();
            logFile_.append(msg, len);
        }
    }
}

void AsyncLog::flush()
{
    addLog(nullptr);
}

void AsyncLog::addLog(std::shared_ptr<LogStream::Buffer> buffer)
{
    bufferQueue_.put(buffer);
}