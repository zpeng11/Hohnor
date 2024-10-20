#include "AsyncLogging.h"
#include <functional>
#include "hohnor/common/Types.h"

using namespace Hohnor;

AsyncLog::AsyncLog():logThread_(std::bind(&AsyncLog::logThreadFunc, this), "AsyncLogThread"),
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
            output_flush();
        }
        else   //normal data
        {
            auto msg = buffer->data();
            auto len = buffer->length();
            output_append(msg, len);
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