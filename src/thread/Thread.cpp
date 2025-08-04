/**
 * Wrapper for pthread 
 */
#include "hohnor/thread/Thread.h"
#include <unistd.h>
#include <exception>
#include "hohnor/thread/Exception.h"
#ifndef __CYGWIN__
#include <sys/prctl.h>
#endif

namespace Hohnor
{
	/**
	 * Data carrier struct, helps void*(void*) interface pass parameters. 
	 * It takes reference to member data of Thread class, allows the owner of the object update threadinfo
	 */
	struct ThreadData
	{
		pid_t &tid_;
		Thread::ThreadFunc func_;
		std::string name_;
		CountDownLatch &latch_;
		explicit ThreadData(Thread::ThreadFunc func, const string &name, pid_t &tid, CountDownLatch &latch) : tid_(tid), func_(func), name_(name), latch_(latch) {}
	};
	/**
	 * We use this function as a starter which fits with function interface void*(void*) that pthread requires,
	 * this is how we make workaround between the Thread interface that accepts std::function<void()> and phread requirement
	 * In short, this is a function for pthread to call
	 */
	void *threadStarter(void *args)
	{
		ThreadData *data = static_cast<ThreadData *>(args);

		data->tid_ = CurrentThread::tid();
		data->latch_.countDown();
		CurrentThread::t_threadName = data->name_.empty() ? "HohnorThread" : data->name_;
#ifndef __CYGWIN__
		::prctl(PR_SET_NAME, CurrentThread::t_threadName);
#endif
		
		try
		{
			data->func_();
			CurrentThread::t_threadName = "finished";
		}
		catch (const Exception &ex)
		{
			CurrentThread::t_threadName = "crashed";
			fprintf(stderr, "exception caught in Thread %s\n", data->name_.c_str());
			fprintf(stderr, "reason: %s\n", ex.what());
			fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
			abort();
		}
		catch (const std::exception &ex)
		{
			CurrentThread::t_threadName = "crashed";
			fprintf(stderr, "exception caught in Thread %s\n", data->name_.c_str());
			fprintf(stderr, "reason: %s\n", ex.what());
			abort();
		}
		catch (...)
		{
			CurrentThread::t_threadName = "crashed";
			fprintf(stderr, "unknown exception caught in Thread %s\n", data->name_.c_str());
			throw; // rethrow
		}

		//std::cout<<"before delete countdown status:"<<data->latch_.getCount()<<std::endl;
		delete data;
		return NULL;
	}

}

/**
 * Static variable that needs sperate space in .cpp
 */
std::atomic_int32_t Hohnor::Thread::numCreated_;

Hohnor::Thread::Thread(ThreadFunc func, const std::string name) : started_(false), joined_(false), pthreadId_(0), tid_(0), func_(func), name_(name), latch_(1)
{
	int num = ++(this->numCreated_);
	if (name_.empty())
	{
		name_ = "HohnorThread" + std::to_string(num);
	}
}

void Hohnor::Thread::start()
{
	assert(!started_);
	started_ = true;
	ThreadData *data = new ThreadData(func_, name_, tid_, latch_);
	if (pthread_create(&pthreadId_, NULL, threadStarter, data))
	{
		started_ = false;
		delete data;
		std::cout << "Failed in pthread_create" << std::endl;
	}
	else
	{
		latch_.wait();//Sync with execution inside the opened thread
		assert(tid_ > 0);
	}
}

int Hohnor::Thread::join()
{
	assert(started_);
	assert(!joined_);
	joined_ = true;
	return pthread_join(pthreadId_, NULL);
}

Hohnor::Thread::~Thread()
{
	if (started_ && !joined_)
	{
		pthread_detach(pthreadId_);
	}
}
