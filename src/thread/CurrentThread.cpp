/**
 * Thread_local scope information that records thread-id and thread name
 */
#include "hohnor/thread/CurrentThread.h"

#include <cxxabi.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <time.h>
#include <execinfo.h>
#include <stdlib.h>

namespace Hohnor
{
    namespace CurrentThread
    {
        thread_local int t_tid = 0;
        thread_local std::string t_threadName = "Unkown";
        static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");

        int tid()
        {
            if (UNLIKELY(t_tid == 0))
                t_tid = static_cast<pid_t>(::syscall(SYS_gettid));
            return t_tid;
        }

        const std::string &name()
        {
            return t_threadName;
        }

        bool isMainThread()
        {
            return tid() == ::getpid();
        }

        void sleepUsec(int64_t usec)
        {
            struct timespec ts = {0, 0};
            const int64_t kMicrosecondsPerSecond = 1000 * 1000;
            ts.tv_sec = static_cast<time_t>(usec / kMicrosecondsPerSecond);
            ts.tv_nsec = static_cast<long>((usec % kMicrosecondsPerSecond) * 1000);
            ::nanosleep(&ts, NULL);
        }
        string stackTrace(bool demangle)
        {
            string stack;
            const int max_frames = 200;
            void *frame[max_frames];
            int nptrs = ::backtrace(frame, max_frames);
            char **strings = ::backtrace_symbols(frame, nptrs);
            if (strings)
            {
                size_t len = 256;
                char *demangled = demangle ? static_cast<char *>(::malloc(len)) : nullptr; //demangle to convert symbols into human readable names
                for (int i = 1; i < nptrs; ++i) // skipping the 0-th, which is this function
                {
                    if (demangle)
                    {
                        // https://panthema.net/2008/0901-stacktrace-demangled/
                        // bin/exception_test(_ZN3Bar4testEv+0x79) [0x401909]
                        char *left_par = nullptr;
                        char *plus = nullptr;
                        for (char *p = strings[i]; *p; ++p)
                        {
                            if (*p == '(')
                                left_par = p;
                            else if (*p == '+')
                                plus = p;
                        }

                        if (left_par && plus)
                        {
                            *plus = '\0';
                            int status = 0;
                            char *ret = abi::__cxa_demangle(left_par + 1, demangled, &len, &status);
                            *plus = '+';
                            if (status == 0)
                            {
                                demangled = ret; // ret could be realloc()
                                stack.append(strings[i], left_par + 1);
                                stack.append(demangled);
                                stack.append(plus);
                                stack.push_back('\n');
                                continue;
                            }
                        }
                    }
                    // Fallback to mangled names
                    stack.append(strings[i]);
                    stack.push_back('\n');
                }
                free(demangled);
                free(strings);
            }
            return stack;
        }
    } //::currentThread

    /*
	The CurrentThread Infomation will be initialized every time a new process is forked from our Thread,
	This will also work for the starter process that has int main()
	*/
    namespace NewProcessAutoHandler
    {
        void afterFork()
        {
            CurrentThread::t_tid = 0;
            CurrentThread::t_threadName = "main";
            CurrentThread::tid();
        }

        class ThreadNameInitializer
        {
        public:
            ThreadNameInitializer()
            {
                CurrentThread::t_threadName = "main";
                CurrentThread::tid();
                pthread_atfork(NULL, NULL, &afterFork);
            }
        };
        ThreadNameInitializer init;
    }
}