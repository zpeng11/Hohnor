/**
 * Thread_local scope information that records thread-id and thread name, when necessary, backtrace thread info
 */
#pragma once

#include "hohnor/common/Types.h"
#include <string>
#include <unistd.h>

namespace Hohnor
{
	namespace CurrentThread
	{
		//Actual varible are in the cpp file
		extern thread_local int t_tid;
		extern thread_local std::string t_threadName;

		int tid();
		const std::string &name();
		bool isMainThread();

		void sleepUsec(int64_t usec);
		string stackTrace(bool demangle);
	}
}