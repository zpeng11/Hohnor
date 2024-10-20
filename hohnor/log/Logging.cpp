#include "hohnor/log/Logging.h"
#include "hohnor/thread/CurrentThread.h"
#include "hohnor/thread/Mutex.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sstream>

namespace Hohnor
{
    //extract file name from the string
    template <int N>
    StringPiece sourceFileName(const char (&arr)[N])
    {
        StringPiece sp;
        sp.set(arr, N - 1);
        const char *slash = strrchr(arr, '/'); // builtin function
        if (slash)
        {
            sp.remove_prefix(slash + 1 - arr);
        }
        return sp;
    }

    //extract file name from the string
    StringPiece sourceFileName(const char *filename)
    {
        const char *slash = strrchr(filename, '/');
        StringPiece sp;
        if (slash)
        {
            sp.set(slash + 1, static_cast<int>(strlen(slash + 1)));
        }
        return sp;
    }

    const char *LogLevelName[Logger::NUM_LOG_LEVELS] =
        {
            "TRACE ",
            "DEBUG ",
            "INFO  ",
            "WARN  ",
            "ERROR ",
            "FATAL ",
    };

    //Log level initilization according to running environment
    Logger::LogLevel initGlobalLogLevel()
    {
        if (::getenv("HOHNOR_LOG_TRACE"))
            return Logger::TRACE;
        else if (::getenv("HOHNOR_LOG_DEBUG"))
            return Logger::DEBUG;
        else
            return Logger::INFO;
    }

    std::shared_ptr<AsyncLog> g_asynclog = std::make_shared<AsyncLogStdout>();
    Logger::LogLevel g_logLevel = initGlobalLogLevel();
    // TimeZone g_logTimeZone;

    thread_local char t_time[64];
    thread_local time_t t_lastSecond;
}

using namespace Hohnor;

void Logger::formatTime()
{
    int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / Timestamp::kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(microSecondsSinceEpoch % Timestamp::kMicroSecondsPerSecond);
    if (seconds != t_lastSecond)
    {
        t_lastSecond = seconds;
        struct tm tm_time;
        ::localtime_r(&seconds, &tm_time); // FIXME TimeZone::fromUtcTime
        int len = snprintf(t_time, sizeof(t_time), "%4d-%02d-%02d %02d:%02d:%02d",
                           tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                           tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        (void)len;
    }
    Fmt us(".%06dZ ", microseconds);
    stream_ << t_time << us.data();
}

void Logger::init(LogLevel level, int savedErrno, const StringPiece &file, int line)
{
    level_ = level;
    time_ = Timestamp::now();
    fileBaseName_ = sourceFileName(file.data());
    line_ = line;
    formatTime();
    stream_ << CurrentThread::name() << ' ';
    if (level >= WARN)
        stream_ << '!' << LogLevelName[level_];
    else
        stream_ << LogLevelName[level_];
    if (savedErrno != 0)
    {
        stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}

Logger::Logger(StringPiece file, int line)
{
    init(INFO, 0, file, line);
}

Logger::Logger(StringPiece file, int line, LogLevel level, const char *func)
{
    init(level, 0, file, line);
    stream_ << "In " << func << "(): ";
}

Logger::Logger(StringPiece file, int line, LogLevel level)
{
    init(level, 0, file, line);
}

Logger::Logger(StringPiece file, int line, bool toAbort)
{
    init(toAbort ? FATAL : ERROR, errno, file, line);
}

Logger::~Logger()
{
    stream_ << " - " << fileBaseName_ << ':' << line_ << '\n';
    g_asynclog->addLog(stream_.moveBuffer());
    if (level_ == FATAL)
    {
        g_asynclog->flush();
        abort();
    }
}

void Logger::setGlobalLogLevel(Logger::LogLevel level)
{
    g_logLevel = level;
}

Logger::LogLevel GlobalLogLevel()
{
    return g_logLevel;
}

void Logger::setAsyncLog(std::shared_ptr<AsyncLog> log)
{
    g_asynclog = log;
}