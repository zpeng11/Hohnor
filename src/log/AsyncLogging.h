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
        explicit AsyncLog();
        ~AsyncLog();
        void addLog(std::shared_ptr<LogStream::Buffer>);
        void flush();
    private:
        void logThreadFunc();
        virtual void output_append(const char * msg, int len){};
        virtual void output_flush(){};
        Thread logThread_;
        BlockingQueue<std::shared_ptr<LogStream::Buffer>> bufferQueue_;
        volatile bool stop_; 
    };

    class AsyncLogStdout : public AsyncLog
    {
    private:
        void output_append(const char * msg, int len){
            size_t n = fwrite(msg, 1, len, stdout);
            assert(n == len && "Write to stdout error");
        }
        void output_flush(){
            fflush(stdout);
        }
    };

    class AsyncLogFile : NonCopyable
    {
    private:
        LogFile logFile_;
    public:
        explicit AsyncLogFile(const string &basename,
                          const string &directory = "./",
                          int checkEveryN = 1024,
                          int flushInterval = 3,
                          off_t rollSize = 65536 * 256, //This is 4 MBytes space by default
                          int rollInterval = 60 * 60 * 24,
                          Timestamp::TimeStandard standrad = Timestamp::UTC): 
                          logFile_(basename, directory, checkEveryN, flushInterval, rollSize, rollInterval, standrad){}
    private:
        void output_append(const char * msg, int len){ logFile_.append(msg, len); }
        void output_flush(){ logFile_.flush(); }
    }; // class AsyncLog
} // namespace Hohnor


