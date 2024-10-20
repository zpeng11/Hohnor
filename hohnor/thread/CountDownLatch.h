/**
 * A latch structure that is simular to singal-slot mechanism, 
 *  consumer(s) wait on the latch for the producer to notify
 */
#pragma once 

#include "hohnor/thread/Condition.h"


namespace Hohnor
{
	class CountDownLatch: NonCopyable
	{
		private:
		int count_;
		Condition cond_;
		Mutex mutex_;
		public:
		explicit CountDownLatch(int countNum):count_(countNum),mutex_(),cond_(mutex_)
		{
		}
		void wait()
		{
			MutexGuard guard(mutex_);
			while(count_>0)
			{
				cond_.wait();
			}
		}
		int getCount()
		{
			MutexGuard guard(this->mutex_);
			return count_;
		}
		void countDown()
		{
			MutexGuard guard(this->mutex_);
			--count_;
			if(count_ == 0)
			{
				cond_.notifyAll();
			}
		}
	};
}