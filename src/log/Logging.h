/**
 * logging interface to log information and dynamic check
 */
//TODO enable message with defferent level to have differnt way of write and flush
#pragma once

#include <memory>
#include "LogStream.h"
#include "Timestamp.h"

namespace Hohnor
{
	/**
	 * This class manages a few global variables like log level, log output function, and log flush function. These variables are used for all threads in a application.
	 * 
	 * It also helps to manage thread-local variables like erro number string representation, time string representation, and time object. These variables only work for current thread.
	 * 
	 * Logger class is servered for LOG and CHECK macros to conviently record run time information, Users most likely do not use this class directly.
	 */
	class Logger
	{
	public:
		enum LogLevel
		{
			TRACE,
			DEBUG,
			INFO,
			WARN,
			ERROR,
			FATAL,
			NUM_LOG_LEVELS,
		};
		Logger(StringPiece file, int line);
		Logger(StringPiece file, int line, LogLevel level);
		Logger(StringPiece file, int line, LogLevel level, const char *func);
		Logger(StringPiece file, int line, bool toAbort);
		~Logger();

		LogStream &stream() { return stream_; }
		LogLevel level() { return level_; }

		//Get global log level
		static LogLevel globalLogLevel();
		//Set global log level
		static void setGlobalLogLevel(LogLevel level);

		//Define callback output function type that deal with moved buffer ptr
		typedef void (*OutputFunc)(std::shared_ptr<LogStream::Buffer> buffer);
		//Define callback flush function type that works with output function
		typedef void (*FlushFunc)();

		//Set the global output callback function
		static void setOutput(OutputFunc);
		//Set the global flush callback function
		static void setFlush(FlushFunc);
		//Set the glocal time zone variable
		//static void setTimeZone(const TimeZone &tz);

	private:
		LogStream stream_;
		Timestamp time_;
		int line_;
		StringPiece fileBaseName_;
		//log level of this piece of log
		LogLevel level_;
		void init(LogLevel level, int savedErrno, const StringPiece &file, int line);
		void formatTime();
		void finish();
	};

	extern Logger::LogLevel g_logLevel;

	inline Logger::LogLevel Logger::globalLogLevel()
	{
		return g_logLevel;
	}

#define LOG_TRACE                                            \
	if (Hohnor::Logger::globalLogLevel() <= Hohnor::Logger::TRACE) \
	Hohnor::Logger(__FILE__, __LINE__, Hohnor::Logger::TRACE, __func__).stream()
#define LOG_DEBUG                                            \
	if (Hohnor::Logger::globalLogLevel() <= Hohnor::Logger::DEBUG) \
	Hohnor::Logger(__FILE__, __LINE__, Hohnor::Logger::DEBUG, __func__).stream()
#define LOG_INFO                                            \
	if (Hohnor::Logger::globalLogLevel() <= Hohnor::Logger::INFO) \
	Hohnor::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN Hohnor::Logger(__FILE__, __LINE__, Hohnor::Logger::WARN).stream()
#define LOG_ERROR Hohnor::Logger(__FILE__, __LINE__, Hohnor::Logger::ERROR).stream()
#define LOG_FATAL Hohnor::Logger(__FILE__, __LINE__, Hohnor::Logger::FATAL).stream()
#define LOG_SYSERR Hohnor::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL Hohnor::Logger(__FILE__, __LINE__, true).stream()

	const char *strerror_tl(int savedErrno);

	//Reference to glog CHECK() macro

#define CHECK(condition) \
	if (!(condition))    \
	(LOG_FATAL) << "'" #condition "' Must be true "

#define CHECK_EQ(lhs, rhs)                        \
	if (lhs != rhs)                               \
	(LOG_FATAL) << "'" #lhs "' Must be equal to " \
				<< "'" #rhs "' "

#define CHECK_NE(lhs, rhs)                         \
	if (lhs == rhs)                                \
	(LOG_FATAL) << "'" #lhs "' Must not equal to " \
				<< "'" #rhs "' "

#define CHECK_NOTNULL(val) \
	CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

	// A small helper for CHECK_NOTNULL() because we want the check function to return ptr for following usage
	template <typename T>
	T *CheckNotNull(StringPiece file, int line, const char *names, T *ptr)
	{
		if (ptr == NULL)
		{
			Logger(file, line, Logger::FATAL).stream() << names;
		}
		return ptr;
	}
}