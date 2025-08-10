#include "hohnor/thread/ThreadPool.h"
#include "hohnor/thread/Exception.h"
#include <memory>
#include <cassert>

namespace Hohnor
{

	ThreadPool::ThreadPool(const string &name) : name_(name), queue_(),
												 preThreadCallback_(), preTaskCallback_(), postTaskCallback_(),
												 pool_(), running_(false),busyThreads_(0)
	{
		//The answer to the universe
		setMaxQueueSize(42);
	}

	ThreadPool::~ThreadPool()
	{
		if (running_)
		{
			stop();
		}
	}

	void ThreadPool::setMaxQueueSize(std::size_t size)
	{
		if(size < 2)
			size = 42;
		queue_ = std::move(std::unique_ptr<BoundedBlockingQueue<Task>>(new BoundedBlockingQueue<Task>(size)));
	}

	void ThreadPool::start(std::size_t threadNum)
	{
		assert(pool_.empty());
		running_ = true;
		pool_.reserve(threadNum);
		for (int i = 0; i < threadNum; i++)
		{
			pool_.emplace_back(new Thread(std::bind(&ThreadPool::runInThread, this), name_ + std::to_string(i)));
			pool_[i]->start();
		}
		if (threadNum == 0 && preThreadCallback_)
		{
			preThreadCallback_();
		}
	}

	/**
	 * Stop the thread will 
	 * 1. Wakeup all threads that are waiting to put tasks in or waiting to take tasks out, takings will have returned nullptr
	 */
	void ThreadPool::stop()
	{
		running_ = false;
		queue_->giveUp();
		for (auto &thr : pool_)
		{
			thr->join();
		}
	}

	void ThreadPool::run(Task task)
	{
		if (UNLIKELY(pool_.empty()))
		{
			task();
		}
		else
		{
			queue_->put(task);
		}
	}

	void ThreadPool::runInThread()
	{
		try
		{
			if (preThreadCallback_)
			{
				preThreadCallback_();
			}
			while (running_)
			{
				Task t = queue_->take();
				busyThreads_++;
				if (LIKELY(t))//See if the task return is nullptr, if so that means the stop() is called
				{
					if (preTaskCallback_)
					{
						preTaskCallback_();
					}
					t();
					if (postTaskCallback_)
					{
						postTaskCallback_();
					}
				}
				busyThreads_--;
			}
			if(postThreadCallback_)
			{
				postThreadCallback_();
			}
		}
		catch (const Exception &ex)
		{
			fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
			fprintf(stderr, "reason: %s\n", ex.what());
			fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
			abort();
		}
		catch (const std::exception &ex)
		{
			fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
			fprintf(stderr, "reason: %s\n", ex.what());
			abort();
		}
		catch (...)
		{
			fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
			throw; // rethrow
		}
	}
}