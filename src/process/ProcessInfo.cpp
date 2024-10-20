#include "ProcessInfo.h"
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <sys/resource.h>
#include <algorithm>
#include "CurrentThread.h"
#include "FileUtils.h"

namespace Hohnor
{
    namespace GlobalInfo
    {
        //starting time of the process, will be initilized when enter the app
        Timestamp g_startTime = Timestamp::now();
        //Clock click per sec
        int g_clockTicks = static_cast<int>(::sysconf(_SC_CLK_TCK));
        //Linux page size
        int g_pageSize = static_cast<int>(::sysconf(_SC_PAGE_SIZE));

        thread_local int t_numOpenedFiles = 0;
        //create file descripter finder that has right form that we can put into scanDir filter
        int fdDirFilter(const struct dirent *d)
        {
            if (::isdigit(d->d_name[0]))
            {
                ++t_numOpenedFiles;
            }
            return 0;
        }
        
        //make the vector global so that it can help the following task filter
        thread_local std::vector<pid_t> *t_pids = NULL;
        //create task finder that has right form that we can put into scanDir filter
        int taskDirFilter(const struct dirent *d)
        {
            if (::isdigit(d->d_name[0]))
            {
                t_pids->push_back(atoi(d->d_name));
            }
            return 0;
        }

        //Wrapper for scanning directory function in <dirent.h>
        int scanDir(const char *dirpath, int (*filter)(const struct dirent *))
        {
            struct dirent **namelist = NULL;
            int result = 0;
#ifndef __CYGWIN__
            result = ::scandir(dirpath, &namelist, filter, alphasort);
#endif //linux only
            assert(namelist == NULL);
            return result;
        }

        //cpu time infomation struct, get from boost.vxworks.hpp
        struct tms
        {
            clock_t tms_utime;  // User CPU time
            clock_t tms_stime;  // System CPU time
            clock_t tms_cutime; // User CPU time of terminated child processes
            clock_t tms_cstime; // System CPU time of terminated child processes
        };
        //cpu time infomation function, get from boost.vxworks.hpp
        clock_t times(struct tms *t)
        {
            struct timespec ts;
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
            clock_t ticks(static_cast<clock_t>(static_cast<double>(ts.tv_sec) * CLOCKS_PER_SEC +
                                            static_cast<double>(ts.tv_nsec) * CLOCKS_PER_SEC / 1000000.0));
            t->tms_utime = ticks / 2U;
            t->tms_stime = ticks / 2U;
            t->tms_cutime = 0; // vxWorks is lacking the concept of a child process!
            t->tms_cstime = 0; // -> Set the wait times for childs to 0
            return ticks;
        }
    }
}

using namespace Hohnor;
using namespace Hohnor::ProcessInfo;
using namespace Hohnor::GlobalInfo;

//Get username of the os
string ProcessInfo::username()
{
    const char *name = "Unknown";
#ifndef __CYGWIN__
    struct passwd pwd;
    struct passwd *result = NULL;
    char buf[8192];
    getpwuid_r(uid(), &pwd, buf, sizeof buf, &result);

    if (result)
    {
        name = pwd.pw_name;
    }
#endif //linux only
    return name;
}

//Timestamp object that has been initilaized since the program started running
Timestamp ProcessInfo::startTime()
{
    return g_startTime;
}

//Clock rate that has been initilaized since the program started running
int ProcessInfo::clockTicksPerSecond()
{
    return g_clockTicks;
}

//Paging size of the os
int ProcessInfo::pageSize()
{
    return g_pageSize;
}

bool ProcessInfo::isDebugBuild()
{
#ifdef NDEBUG
    return false;
#else
    return true;
#endif
}

//host name
string ProcessInfo::hostname()
{
    // HOST_NAME_MAX 64
    // _POSIX_HOST_NAME_MAX 255
    char buf[256];
    if (::gethostname(buf, sizeof buf) == 0)
    {
        buf[sizeof(buf) - 1] = '\0';
        return buf;
    }
    else
    {
        return "Unknown";
    }
}

//get process information
string ProcessInfo::procname()
{
    return procname(procStat()).as_string();
}

//get the name of the current running process
StringPiece ProcessInfo::procname(const string &stat)
{
    StringPiece name;
    size_t lp = stat.find('(');
    size_t rp = stat.rfind(')');
    if (lp != string::npos && rp != string::npos && lp < rp)
    {
        name.set(stat.data() + lp + 1, static_cast<int>(rp - lp - 1));
    }
    return name;
}

//read self process information
string ProcessInfo::procStatus()
{
    string result;
#ifndef __CYGWIN__
    FileUtils::readFile("/proc/self/status", 65536, &result);
#endif //linux only
    return result;
}

//statistic information for current process
string ProcessInfo::procStat()
{
    string result;
#ifndef __CYGWIN__
    FileUtils::readFile("/proc/self/stat", 65536, &result);
#endif //linux only
    return result;
}

string ProcessInfo::threadStat()
{
    char buf[64];
    snprintf(buf, sizeof buf, "/proc/self/task/%d/stat", CurrentThread::tid());
    string result;
#ifndef __CYGWIN__
    
    FileUtils::readFile(buf, 65536, &result);
#endif //linux only
    return result;
}

string ProcessInfo::exePath()
{
    string result;
    char buf[1024];
#ifndef __CYGWIN__
    ssize_t n = ::readlink("/proc/self/exe", buf, sizeof buf);
    if (n > 0)
    {
        result.assign(buf, n);
    }
#endif //linux only
    return result;
}

int ProcessInfo::openedFiles()
{
    t_numOpenedFiles = 0;
    scanDir("/proc/self/fd", fdDirFilter);
    return t_numOpenedFiles;
}

int ProcessInfo::maxOpenFiles()
{
    struct rlimit rl;
    if (::getrlimit(RLIMIT_NOFILE, &rl))
    {
        return openedFiles();
    }
    else
    {
        return static_cast<int>(rl.rlim_cur);
    }
}


ProcessInfo::CpuTime ProcessInfo::cpuTime()
{
    ProcessInfo::CpuTime t;
    struct tms tms;
    if (::times(&tms) >= 0)
    {
        const double hz = static_cast<double>(clockTicksPerSecond());
        t.userSeconds = static_cast<double>(tms.tms_utime) / hz;
        t.systemSeconds = static_cast<double>(tms.tms_stime) / hz;
    }
    return t;
}

int ProcessInfo::numThreads()
{
    int result = 0;
    string status = procStatus();
    size_t pos = status.find("Threads:");
    if (pos != string::npos)
    {
        result = ::atoi(status.c_str() + pos + 8);
    }
    return result;
}

std::vector<pid_t> ProcessInfo::threads()
{
    std::vector<pid_t> result;
    t_pids = &result;
    scanDir("/proc/self/task", taskDirFilter);
    t_pids = NULL;
    std::sort(result.begin(), result.end());
    return result;
}