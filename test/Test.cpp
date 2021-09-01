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
#include "AsyncLogging.h"
#include "SocketWrap.h"
#include "InetAddress.h"
using namespace std;

using namespace Hohnor;

int main(int argc, char *argv[])
{
    sockaddr_in6 addr;
    InetAddress id(addr);
    cout<<InetAddress::resolve("google.com", &id)<<endl;
}