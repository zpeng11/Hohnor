/**
 * Wrapper for pthread mutex and the RAII guard class
 */
#pragma once

#include "CurrentThread.h"
#include "common/NonCopyable.h"
#include <assert.h>
#include <pthread.h>
#include <iostream>

#ifndef NDEBUG
//Check return value of mutex lock, for non-zero return take care of 
#define MCHECK(ret) (                   \
	{                                   \
		__typeof__(ret) errnum = (ret); \
		assert(errnum == 0);            \
		(void)errnum;                   \
	})

#else
#define MCHECK(ret) (ret)
#endif
namespace Hohnor
{
	class Mutex : NonCopyable
	{
	private:
		pthread_mutex_t mutex_t;
		pid_t currentHolder;
		void unassignHolder()
		{
			currentHolder = 0;
		}

		void assignHolder()
		{
			currentHolder = CurrentThread::tid();
		}
		friend class Condition;//allow condition to access holder

	public:
		explicit Mutex() : currentHolder(0)
		{
			MCHECK(pthread_mutex_init(&mutex_t, nullptr));
		}
		~Mutex()
		{
			assert(currentHolder == 0);
			MCHECK(pthread_mutex_destroy(&mutex_t));
		}
		void lock()
		{
			MCHECK(pthread_mutex_lock(&mutex_t));
			assignHolder();
		}
		void unlock()
		{
			MCHECK(pthread_mutex_unlock(&mutex_t));
			unassignHolder();
		}
		pid_t lockingThread()
		{
			assert(currentHolder != 0);
			return currentHolder;
		}
		bool isLockedByThisThread() const
		{
			return currentHolder == CurrentThread::tid();
		}

		inline void assertLocked() const
		{
			assert(isLockedByThisThread());
		}

		pthread_mutex_t *getPthreadMutex() /* non-const */
		{
			return &mutex_t;
		}
	};
	/**
	 * Use RAII to manage lock and unlock
	 */
	class MutexGuard : NonCopyable
	{
	private:
		Mutex &mutexlock;

	public:
		explicit MutexGuard(Mutex &mutex) : mutexlock(mutex)
		{
			mutexlock.lock();
		}
		~MutexGuard()
		{
			mutexlock.unlock();
		}
	};
}