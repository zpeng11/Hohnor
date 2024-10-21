#include <gtest/gtest.h>
#include <functional>
#include <memory>
#include "hohnor/thread/Thread.h"
#include "hohnor/thread/BlockingQueue.h"
using namespace Hohnor;


//GTest asserts are thread-safe that is good :)
TEST(threadtest, runmethods) {
  const int num = 1000;
  pid_t pids_from_thread[num] = {0};
  int updateData[num] = {0};
  std::vector<std::shared_ptr<Thread>> threads;
  for(int i = 0 ; i < num; i++){
    auto fuc = [&pids_from_thread,&updateData, i](){
        pids_from_thread[i] = CurrentThread::tid();
        updateData[i] = i;
    };
    auto thread = std::make_shared<Thread>(fuc);
    thread->start();
    threads.push_back(thread);
    EXPECT_EQ(Thread::numCreated(), i+1);
  }
  for(int i= 0; i < num; i++) threads[i]->join();
  for(int i= 0; i < num; i++){
    EXPECT_EQ(pids_from_thread[i], threads[i]->tid());
    EXPECT_EQ(updateData[i] , i);
  }
}

TEST(threadtest, puducerconsumer){
    const int num_of_tasks = 1000;
    BlockingQueue<int> queue;
    int recCnt = 0;
    auto producorFuc = [num_of_tasks, &queue]{
        for(int i = num_of_tasks-1; i >=0; i-- ){//put 0 as last one
            usleep(1000);
            queue.put(i);
        }
    };
    auto consumerFuc = [&queue, &recCnt]{
        int rec = -1;
        do{
            rec = queue.take();
            recCnt++;
        }while(rec);
    };
    Thread prod(producorFuc);
    Thread cons(consumerFuc);
    prod.start();
    cons.start();
    prod.join();
    cons.join();
    EXPECT_EQ(recCnt, num_of_tasks);
}