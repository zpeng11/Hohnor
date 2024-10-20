/**
 * Wrapper for pthread's thread
 */
#pragma once

#include "CountDownLatch.h"
#include "CurrentThread.h"
#include "pthread.h"
#include <functional>
#include <string>
#include <atomic>

namespace Hohnor
{
	class Thread : NonCopyable
	{
	public:
	/**
	 * Usually works with lambda functions.
	 */
		typedef std::function<void()> ThreadFunc;

	private:
		bool started_;
		bool joined_;
		pthread_t pthreadId_;
		pid_t tid_;
		ThreadFunc func_;
		std::string name_;
		CountDownLatch latch_;
		static std::atomic_int32_t numCreated_;

	public:
	/**
	 * 4 essentail thread operations;
	 */
		Thread(ThreadFunc func, const std::string name = std::string());
		void start();
		int join();
		~Thread();
	/**
	 * Supplementary information that helps the Thread object holder to acknowlege information
	 */
		bool started() const { return started_; }
		pthread_t pthreadId() const { return pthreadId_; }
		pid_t tid() const { return tid_; }
		const string &name() const { return name_; }

		static int numCreated() { return numCreated_.load(); }
	};
}