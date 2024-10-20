/**
 * Exeptions that can print stack trace information
 */
#pragma once

#include "hohnor/thread/CurrentThread.h"

namespace Hohnor
{
	class Exception : public std::exception
	{
	public:
		Exception(string msg) : message_(std::move(msg)),
								stack_(CurrentThread::stackTrace(/*demangle=*/false)) {}
		~Exception() noexcept override = default;

		// default copy-ctor and operator= are okay.

		const char *what() const noexcept override
		{
			return message_.c_str();
		}

		const char *stackTrace() const noexcept
		{
			return stack_.c_str();
		}

	private:
		string message_;
		string stack_;
	};
}