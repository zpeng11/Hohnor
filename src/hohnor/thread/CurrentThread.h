/**
 * Thread_local scope information that records thread-id and thread name
 * This is useful for debug and logging
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
		/*
		* For tracing stack in the exception， we know that each thread has its own stack, if we want to trace an exception
		* We will need to apply this, I also provide demangle feature from 
		* https://panthema.net/2008/0901-stacktrace-demangled/
		* We need demangle feature because different from C, C++ would try to change function names and make stack trace information
		* Very unreadable.
		* BUt using __cxa_demangle() feature from libstdc++ library, we are able to recover readable name for stack.
		* Notice this only works for GUN g++ compiler
		*/
		string stackTrace(bool demangle);
	}
}