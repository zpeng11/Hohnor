#include <iostream>
#include <stdio.h>
#include "Thread.h"
#include "Timestamp.h"
#include "Date.h"
#include "Logging.h"
#include "FileUtils.h"
#include "ProcessInfo.h"
#include "ThreadPool.h"
#include <memory>
#include "LogFile.h"
#include "LogFile.h"
using namespace std;


Hohnor::ThreadPool tp;
Hohnor::Mutex m;

using namespace std;

shared_ptr<int> ptrfunc()
{
	return std::move(shared_ptr<int>(new int(200)));
}


int main()
{
	auto ptr = ptrfunc();
	cout<<*ptr<<endl;
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
	Hohnor::LogFile lf("append.txt");
	char str[] = "Helloworld";
	string str;
	lf.append(str, strlen(str));
	sleep(1);
}