#include <iostream>
#include <stdio.h>
#include "Thread.h"
#include "Timestamp.h"
#include "Date.h"
#include "Logging.h"
using namespace std;





int main()
{
	Hohnor::Thread t([]()
	{
		cout<<Hohnor::CurrentThread::tid()<<endl;
		cout<<Hohnor::CurrentThread::name()<<endl;
		cout<<Hohnor::Timestamp::now().toString()<<endl;
		cout<<Hohnor::Date::kJulianDayOf1970_01_01<<endl;
		CHECK_EQ(12,13)<<"Hello world";
	});
	t.start();
	sleep(1);
}