#pragma once
#include "Thread.h"
#include "BoundedBlockingQueue.h"
#include <vector>
#include <memory>

namespace Hohnor
{
	/**
	 * The threadpool is consist of certain number of running threads, these threads take std::function tasks
	 * 
	 * Threads repeat [run the task]->[get a task from queue]->[run the task]->[get a task from queue]...
	 * 
	 * So there is a vector of running Threads, and a queue that buffs input tasks,
	 * I choose BoundedBlocking Queue, which builds on circular buffer which is faster than linked list implementation.
	 */
	class ThreadPool : NonCopyable
	{
	
	public:
		typedef std::function<void()> Task;
		explicit ThreadPool(const string &name = string("ThreadPool"));
		~ThreadPool();
		void setPreThreadCallback(const Task &task){preThreadCallback_ = task;}
		void setPostThreadCallback(const Task &task){postThreadCallback_ = task;}
		//Set function runs before every task runs
		void setPreTaskCallback(const Task &task){preTaskCallback_ = task;}
		//Set function runs after every task runs
		void setPostTaskCallback(const Task &task){postTaskCallback_ = task;}
		//Set the waiting queue to have limited size using BoundedBlockingQueue, not allowed to be less than 2
		void setMaxQueueSize(std::size_t size);
		//Start and set number of threads to run
		void start(std::size_t threadNum);
		//Put a task into the waiting queue !!May not run immediately. This call may be blocked to wait for queue not full.
		void run(Task task);
		//Stop retrieving tasks from queue, and Wait for all threads to finish their current work and join.
		void stop();
		//Get pool name
		const string& name() const{ return name_; }
		//Capacity of the queue
		std::size_t queueCapacity() const{return queue_->capacity();}
		//Size of current queue
		std::size_t queueSize()const {return queue_->size();}
		//If the queue is full
		bool full()const{return queue_->full();}
		//Number of running threads
		std::size_t poolSize() const{return pool_.size();}
		//Get busy threads
		std::size_t busyThreads() {return busyThreads_;}

	private:
		std::string name_;
		std::unique_ptr<BoundedBlockingQueue<Task>> queue_;
		Task preThreadCallback_;
		Task postThreadCallback_;
		Task preTaskCallback_;
		Task postTaskCallback_;
		std::vector<std::unique_ptr<Thread>> pool_;
		std::atomic<std::size_t> busyThreads_;
		bool running_;
		void runInThread();
	};
}