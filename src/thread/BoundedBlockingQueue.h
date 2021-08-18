#pragma once

#include "Mutex.h"
#include "Condition.h"
#include "CircularBuffer.h"

namespace Hohnor
{
	template <typename T>
	class BoundedBlockingQueue : Hohnor::NonCopyable
	{
	public:
		explicit BoundedBlockingQueue(int maxSize)
			: mutex_(),
			  notEmpty_(mutex_),
			  notFull_(mutex_),
			  queue_(maxSize)
		{
		}

		void put(const T &x)
		{
			MutexGuard lock(mutex_);
			while (queue_.full())
			{
				notFull_.wait();
			}
			assert(!queue_.full());
			queue_.enqueue(x);
			notEmpty_.notify();
		}

		void put(T &&x)
		{
			MutexGuard lock(mutex_);
			while (queue_.full())
			{
				notFull_.wait();
			}
			assert(!queue_.full());
			queue_.enqueue(std::move(x));
			notEmpty_.notify();
		}

		T take()
		{
			MutexGuard lock(mutex_);
			while (queue_.empty())
			{
				notEmpty_.wait();
			}
			assert(!queue_.empty());
			T front(std::move(queue_.dequeue()));
			notFull_.notify();
			return front;
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

		size_t size() const
		{
			MutexGuard lock(mutex_);
			return queue_.size();
		}

		size_t capacity() const
		{
			MutexGuard lock(mutex_);
			return queue_.capacity();
		}

	private:
		mutable Mutex mutex_;
		Condition notEmpty_ ;
		Condition notFull_ ;
		CircularBuffer<T> queue_;
	};
}