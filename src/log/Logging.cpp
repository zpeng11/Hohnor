#include "Logging.h"
#include "CurrentThread.h"
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

	void defaultOutput(const char *msg, int len)
	{
		size_t n = fwrite(msg, 1, len, stdout);
		assert(n == len);
	}

	void defaultFlush()
	{
		fflush(stdout);
	}

	//Log level initilization according to running environment
	Logger::LogLevel initLogLevel()
	{
		if (::getenv("HOHNOR_LOG_TRACE"))
			return Logger::TRACE;
		else if (::getenv("HOHNOR_LOG_DEBUG"))
			return Logger::DEBUG;
		else
			return Logger::INFO;
	}

	Logger::OutputFunc g_output = defaultOutput;
	Logger::FlushFunc g_flush = defaultFlush;
	Logger::LogLevel g_logLevel = initLogLevel();
	// TimeZone g_logTimeZone;

	thread_local char t_errnobuf[512];
	thread_local char t_time[64];
	thread_local time_t t_lastSecond;

	//Use thread safe strerror call to safe erro number information in thread-local variable
	const char *strerror_tl(int savedErrno)
	{
		#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE
		strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
		return t_errnobuf;
		#else
		return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
		#endif
	}
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
		// if (g_logTimeZone.valid())
		// {
		// 	tm_time = g_logTimeZone.toLocalTime(seconds);
		// }
		// else
		// {
			::gmtime_r(&seconds, &tm_time); // FIXME TimeZone::fromUtcTime
		// }
		int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
						   tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
						   tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
		assert(len == 17);
		(void)len;
	}
	// if (g_logTimeZone.valid())
	// {
	// 	Fmt us(".%06d ", microseconds);
	// 	assert(us.length() == 8);
	// 	stream_ << t_time << us.data();
	// }
	// else
	// {
		Fmt us(".%06dZ ", microseconds);
		assert(us.length() == 9);
		stream_ << t_time << us.data();
	// }
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
	stream_ << func << ' ';
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
	const LogStream::Buffer &buf(stream().buffer());
	g_output(buf.data(), buf.length());
	if (level_ == FATAL)
	{
		g_flush();
		abort();
	}
}

void Logger::setLogLevel(Logger::LogLevel level)
{
	g_logLevel = level;
}

void Logger::setOutput(OutputFunc out)
{
	g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
	g_flush = flush;
}

// void Logger::setTimeZone(const TimeZone &tz)
// {
// 	g_logTimeZone = tz;
// }