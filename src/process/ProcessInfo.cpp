#include "ProcessInfo.h"
#include <sys/dirent.h>
#include <sys/resource.h>
#include "CurrentThread.h"

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
        int fdDirFilter(const struct dirent *d)
        {
            if (::isdigit(d->d_name[0]))
            {
                ++t_numOpenedFiles;
            }
            return 0;
        }

        thread_local std::vector<pid_t> *t_pids = NULL;
        int taskDirFilter(const struct dirent *d)
        {
            if (::isdigit(d->d_name[0]))
            {
                t_pids->push_back(atoi(d->d_name));
            }
            return 0;
        }

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
    }
}

using namespace Hohnor;
using namespace Hohnor::ProcessInfo;
using namespace Hohnor::GlobalInfo;

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

Timestamp ProcessInfo::startTime()
{
    return g_startTime;
}

int ProcessInfo::clockTicksPerSecond()
{
    return g_clockTicks;
}

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

string ProcessInfo::procname()
{
    return procname(procStat()).as_string();
}

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

string ProcessInfo::procStatus()
{
    string result;
#ifndef __CYGWIN__
    FileUtil::readFile("/proc/self/status", 65536, &result);
#endif //linux only
    return result;
}

string ProcessInfo::procStat()
{
    string result;
#ifndef __CYGWIN__
    FileUtil::readFile("/proc/self/stat", 65536, &result);
#endif //linux only
    return result;
}

string ProcessInfo::threadStat()
{
    char buf[64];
    snprintf(buf, sizeof buf, "/proc/self/task/%d/stat", CurrentThread::tid());
    string result;
#ifndef __CYGWIN__
    FileUtil::readFile(buf, 65536, &result);
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