/**
 * Thread_local scope information that records thread-id and thread name
 */
#pragma once

#include "Types.h"
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

	//Use thread safe strerror call to safe erro number information 
	inline string strerror_tl(int savedErrno)
    {
		char t_errnobuf[512];
		#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE
		strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
		return t_errnobuf;
		#else
		return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
		#endif
	}
}