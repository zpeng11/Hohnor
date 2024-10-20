/**
 * A more advanced Blocking queue, it has bounded size limit, thread safe.
 * The basic level uses circular buffer to implement, since it has scheduled fixed memory,
 * it works faster than normal queue, 
 * the drawback is that size of the queue should be larger than 1
 * (otherwise ring buffer will be hard to implement)
 */
#pragma once

#include "hohnor/thread/Mutex.h"
#include "hohnor/thread/Condition.h"
#include "hohnor/common/CircularBuffer.h"
#include "hohnor/common/SyncQueue.h"
#include <atomic>

namespace Hohnor
{
	template <typename T>
	class BoundedBlockingQueue : public SyncQueue<T>
	{
	public:
		explicit BoundedBlockingQueue(int maxSize)
			: mutex_(),
			  notEmpty_(mutex_),
			  notFull_(mutex_),
			  queue_(maxSize),
              end_(false)
		{
		}

		void put(const T &x) 
		{
			MutexGuard lock(mutex_);
			while (queue_.full() &&!end_)
			{
				notFull_.wait();
			}
			if(UNLIKELY(end_))
			{
				return;
			}
			assert(!queue_.full());
			queue_.enqueue(x);
			notEmpty_.notify();
		}

		void put(T &&x) 
		{
			MutexGuard lock(mutex_);
			while (queue_.full() &&!end_)
			{
				notFull_.wait();
			}
			if(UNLIKELY(end_))
			{
				return;
			}
			assert(!queue_.full());
			queue_.enqueue(std::move(x));
			notEmpty_.notify();
		}

		T take() 
		{
			MutexGuard lock(mutex_);
			while (queue_.empty() && !end_)
			{
				notEmpty_.wait();
			}
			if(UNLIKELY(end_))
			{
				return T();
			}
			assert(!queue_.empty());
			T front(std::move(queue_.dequeue()));
			notFull_.notify();
			return std::move(front);
		}

		bool empty() const
		{
			MutexGuard lock(mutex_);
			return queue_.empty();
		}

		bool full() const
		{
			MutexGuard lock(mutex_);
			return queue_.full();
		}

		std::size_t size() const 
		{
			MutexGuard lock(mutex_);
			return queue_.size();
		}

		std::size_t capacity() const
		{
			MutexGuard lock(mutex_);
			return queue_.capacity();
		}
		//Give up all contains inside the queue, and disable the functionality of putting and taking
		//The blocking take() will return defualt contructed T object.
		//(please do not use the object after calling)
		void giveUp() 
		{
			MutexGuard guard(mutex_);
			end_ = true;
			notEmpty_.notifyAll();
			notFull_.notifyAll();
		}

	private:
		mutable Mutex mutex_;
		Condition notEmpty_ ;
		Condition notFull_ ;
		CircularBuffer<T> queue_;
		std::atomic<bool> end_ ;
	};
}