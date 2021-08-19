#pragma once
#include "Thread.h"
#include "BoundedBlockingQueue.h"
#include <vector>
#include <memory>

namespace Hohnor
{
	/**
	 * The threadpool is consist of certain number of running threads,.
	 * 
	 * Threads repeat [run the task]->[get a task from queue]->[run the task]->[get a task from queue]...
	 * 
	 * It is easy to implement this process, but most difficult things for thread pool is to stop the pool.
	 * First, all running tasks should jump out of infinite loops. This will be for the user to handle.
	 * Second, threads should stop fetching task from the queue and join to the main thread.
	 * 
	 * The giveUp() method is the method we can use to solve
	 * 
	 */
	class ThreadPool : NonCopyable
	{
	
	public:
		typedef std::function<void()> Task;
		explicit ThreadPool(const string &name = string("ThreadPool"));
		~ThreadPool();
		void setPreThreadCallback(const Task &task){preThreadCallback_ = task;}
		//Set function runs before every task runs
		void setPreTaskCallback(const Task &task){preTaskCallback_ = task;}
		//Set function runs after every task runs
		void setPostTaskCallback(const Task &task){postTaskCallback_ = task;}
		//Set the waiting queue to have limited size using BoundedBlockingQueue, not allowed to be less than 2
		void setMaxQueueSize(std::size_t size);
		//Start and set number of threads to run
		void start(std::size_t threadNum);
		//Put a task into the thread pool !!May not run immediately
		void run(Task task);
		//Wait for all threads to finish their work
		void stop();
		//Get pool name
		const string& name() const{ return name_; }
		//Inquiry number of waiting tasks
		std::size_t queueSize() const{return queue_->capacity();}
		//Number of running threads
		std::size_t poolSize() const{return pool_.size();}
	private:
		std::string name_;
		std::unique_ptr<BoundedBlockingQueue<Task>> queue_;
		Task preThreadCallback_;
		Task preTaskCallback_;
		Task postTaskCallback_;
		std::vector<std::unique_ptr<Thread>> pool_;
		bool running_;
		void runInThread();
	};
}