#include <iostream>
#include <stdio.h>
#include "Thread.h"
#include "Timestamp.h"
#include "Date.h"
#include "Logging.h"
#include "FileUtils.h"
#include "ProcessInfo.h"
#include "ThreadPool.h"
using namespace std;


Hohnor::ThreadPool tp;
Hohnor::Mutex m;

int main()
{
	auto func = [&]()
	{
		Hohnor::MutexGuard g(m);
		LOG_INFO<<"Enter thread";
		cout<<"thread name:"<<Hohnor::CurrentThread::name()<<endl;
		cout<<"Time now:"<<Hohnor::Timestamp::now().toFormattedString()<<endl;
		cout<<"Hostname:"<<Hohnor::ProcessInfo::hostname()<<endl;
	};
	tp.setPreThreadCallback([&](){cout<<"--------------\n\n"<<endl;});
	tp.start(4);
	for(int i = 0 ;i<20;i++)
	tp.run(func);
	sleep(1);
}