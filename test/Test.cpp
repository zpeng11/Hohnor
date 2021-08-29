#include <iostream>
#include <stdio.h>
#include "Thread.h"
#include "Timestamp.h"
#include "Date.h"
#include "Logging.h"
#include "FileUtils.h"
#include "ProcessInfo.h"
#include "ThreadPool.h"
#include "LogFile.h"
using namespace std;


Hohnor::ThreadPool tp;
Hohnor::Mutex m;

int main()
{
	Hohnor::g_logLevel = Hohnor::Logger::TRACE;
	auto func = [&]()
	{
		Hohnor::MutexGuard g(m);
		LOG_DEBUG <<"Enter thread";
		cout<<"thread name:"<<Hohnor::CurrentThread::name()<<endl;
		cout<<"Hostname:"<<Hohnor::ProcessInfo::hostname()<<endl;
		cout<<"Max open:"<<Hohnor::ProcessInfo::maxOpenFiles()<<endl;
	};
	tp.setPostTaskCallback([&](){cout<<"--------------\n\n"<<endl;});
	LOG_DEBUG<<"Hello world";
	// tp.start(4);
	// for(int i = 0 ;i<20;i++)
	// tp.run(func);
	sleep(1);
}