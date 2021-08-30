#pragma once
#include <memory>
#include "Types.h"
#include "NonCopyable.h"
#include "BlockingQueue.h"
#include "Thread.h"
#include "LogStream.h"
#include "LogFile.h"

namespace Hohnor
{
    class AsyncLog : NonCopyable
    {
    public:
        explicit AsyncLog(const string &basename,
                          const string &directory = "./",
                          int checkEveryN = 1024,
                          int flushInterval = 3,
                          off_t rollSize = 65535,
                          int rollInterval = 60 * 60 * 24,
                          Timestamp::TimeStandard standrad = Timestamp::UTC);
        ~AsyncLog();
        void addLog(std::shared_ptr<LogStream::Buffer>);
        void flush();

    private:
        void logThreadFunc();
        LogFile logFile_;
        Thread logThread_;
        BlockingQueue<std::shared_ptr<LogStream::Buffer>> bufferQueue_;
        volatile bool stop_;
    }; // class AsyncLog
} // namespace Hohnor


