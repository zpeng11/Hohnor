#pragma once

#include "NonCopyable.h"
#include "Mutex.h"
#include "Condition.h"
#include <deque>

namespace Hohnor
{
	template <typename T>
	class BlockingQueue: NonCopyable
	{
	public:
		using queue_type = std::deque<T>;

		BlockingQueue()
			: mutex_(),
			  notEmpty_(mutex_),
			  queue_()
		{
		}

		void put(const T &x)
		{
			MutexGuard lock(mutex_);
			queue_.push_back(x);
			notEmpty_.notify(); // wait morphing saves us
								// http://www.domaigne.com/blog/computing/condvars-signal-with-mutex-locked-or-not/
		}

		void put(T &&x)
		{
			MutexGuard lock(mutex_);
			queue_.push_back(std::move(x));
			notEmpty_.notify();
		}

		T take()
		{
			MutexGuard lock(mutex_);
			// always use a while-loop, due to spurious wakeup
			while (queue_.empty())
			{
				notEmpty_.wait();
			}
			assert(!queue_.empty());
			T front(std::move(queue_.front()));
			queue_.pop_front();
			return front;
		}

		queue_type drain()
		{
			std::deque<T> queue;
			{
				MutexGuard lock(mutex_);
				queue = std::move(queue_);
				assert(queue_.empty());
			}
			return queue;
		}

		size_t size() const
		{
			MutexGuard lock(mutex_);
			return queue_.size();
		}

	private:
		mutable Mutex mutex_;
		Condition notEmpty_;
		queue_type queue_ ;
	};

}