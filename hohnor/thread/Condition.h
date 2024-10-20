/**
 * wrapper of conditional variable in pthread
 */
#pragma once

#include "Mutex.h"
#include "pthread.h"

namespace Hohnor
{
	/**
	 * wrap for pthread conditional
	 */
	class Condition : NonCopyable
	{
	private:
		Mutex &mutexlock;
		pthread_cond_t cond;

	public:
		explicit Condition(Mutex &lock) : mutexlock(lock)
		{
			MCHECK(pthread_cond_init(&cond, nullptr));
		}
		~Condition()
		{
			MCHECK(pthread_cond_destroy(&cond));
		}
		void wait()
		{
			mutexlock.unassignHolder(); //The holder property is not useful when waiting cond due to cond's design idea
			MCHECK(pthread_cond_wait(&cond, mutexlock.getPthreadMutex()));
			mutexlock.assignHolder();
		}
		bool timedWait(double seconds)
		{
			struct timespec abstime;
			clock_gettime(CLOCK_REALTIME, &abstime);

			const int64_t kNanoSecondsPerSecond = 1000000000;
			int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);

			abstime.tv_sec += static_cast<time_t>((abstime.tv_nsec + nanoseconds) / kNanoSecondsPerSecond);
			abstime.tv_nsec = static_cast<long>((abstime.tv_nsec + nanoseconds) % kNanoSecondsPerSecond);

			mutexlock.unassignHolder();//same as wait, disable holder property
			bool res = ETIMEDOUT == pthread_cond_timedwait(&cond, mutexlock.getPthreadMutex(), &abstime);
			mutexlock.assignHolder();
			return res;
		}
		void notify()
		{
			MCHECK(pthread_cond_signal(&cond));
		}

		void notifyAll()
		{
			MCHECK(pthread_cond_broadcast(&cond));
		}
	};
}