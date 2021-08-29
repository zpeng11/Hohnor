#pragma once

#include "StringPiece.h"
#include "Types.h"
#include "Timestamp.h"
#include <vector>
#include <unistd.h>

namespace Hohnor
{
    namespace ProcessInfo
    {
        inline pid_t pid() { return getpid(); }
        inline string pidString() { return std::to_string(pid()); }
        inline uid_t uid() { return getuid(); }
        string username();
        inline uid_t euid() { return geteuid(); }
        Timestamp startTime();
        int clockTicksPerSecond();
        int pageSize();
        bool isDebugBuild(); 
        string hostname();


        string procname();
        StringPiece procname(const string &stat);

        /// read /proc/self/status
        string procStatus();

        /// read /proc/self/stat
        string procStat();

        /// read /proc/self/task/tid/stat
        string threadStat();

        /// readlink /proc/self/exe
        string exePath();

        int openedFiles();
        int maxOpenFiles();

        struct CpuTime
        {
            double userSeconds;
            double systemSeconds;

            CpuTime() : userSeconds(0.0), systemSeconds(0.0) {}

            double total() const { return userSeconds + systemSeconds; }
        };
        CpuTime cpuTime();

        int numThreads();
        std::vector<pid_t> threads();
    } // namespace ProcessInfo
} // namespace Hohnor