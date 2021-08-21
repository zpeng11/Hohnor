#include "Logging.h"
#include "../thread/CurrentThread.h"

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
		const char *slash = strrchr(data_, '/'); // builtin function
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

	thread_local char t_errnobuf[512];
	thread_local char t_time[64];
	thread_local time_t t_lastSecond;

	//Use thread safe strerror call to safe erro number information in thread-local variable
	const char *strerror_tl(int savedErrno)
	{
		return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
	}
}

using namespace Hohnor;
