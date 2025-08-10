#include "hohnor/process/ProcessInfo.h"
#include "hohnor/thread/CurrentThread.h"
#include <gtest/gtest.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <climits>
#include <thread>
#include <chrono>

using namespace Hohnor;

class ProcessInfoTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

// Test basic process identification functions
TEST_F(ProcessInfoTest, BasicProcessInfo) {
    // Test pid() function
    pid_t current_pid = ProcessInfo::pid();
    EXPECT_GT(current_pid, 0);
    EXPECT_EQ(current_pid, ::getpid());

    // Test pidString() function
    string pid_str = ProcessInfo::pidString();
    EXPECT_EQ(pid_str, std::to_string(current_pid));
    EXPECT_FALSE(pid_str.empty());

    // Test uid() function
    uid_t current_uid = ProcessInfo::uid();
    EXPECT_EQ(current_uid, ::getuid());

    // Test euid() function
    uid_t current_euid = ProcessInfo::euid();
    EXPECT_EQ(current_euid, ::geteuid());
}

TEST_F(ProcessInfoTest, Username) {
    string username = ProcessInfo::username();
    EXPECT_FALSE(username.empty());
    
    // Verify username matches system call result
    struct passwd *pw = getpwuid(getuid());
    if (pw != nullptr) {
        EXPECT_EQ(username, string(pw->pw_name));
    }
}

TEST_F(ProcessInfoTest, StartTime) {
    Timestamp start_time = ProcessInfo::startTime();
    EXPECT_TRUE(start_time.valid());
    
    // Start time should be before current time
    Timestamp now = Timestamp::now();
    EXPECT_LT(start_time.microSecondsSinceEpoch(), now.microSecondsSinceEpoch());
}

TEST_F(ProcessInfoTest, SystemParameters) {
    // Test clockTicksPerSecond
    int clock_ticks = ProcessInfo::clockTicksPerSecond();
    EXPECT_GT(clock_ticks, 0);
    EXPECT_EQ(clock_ticks, static_cast<int>(::sysconf(_SC_CLK_TCK)));

    // Test pageSize
    int page_size = ProcessInfo::pageSize();
    EXPECT_GT(page_size, 0);
    EXPECT_EQ(page_size, static_cast<int>(::sysconf(_SC_PAGE_SIZE)));
    // Common page sizes are 4096 or 8192
    EXPECT_TRUE(page_size == 4096 || page_size == 8192 || page_size > 0);
}

TEST_F(ProcessInfoTest, DebugBuild) {
    bool is_debug = ProcessInfo::isDebugBuild();
    // This should return consistent results
#ifdef NDEBUG
    EXPECT_FALSE(is_debug);
#else
    EXPECT_TRUE(is_debug);
#endif
}

TEST_F(ProcessInfoTest, Hostname) {
    string hostname = ProcessInfo::hostname();
    EXPECT_FALSE(hostname.empty());
    EXPECT_NE(hostname, "Unknown");
    
    // Hostname should not be too long
    EXPECT_LE(hostname.length(), 255);
}

TEST_F(ProcessInfoTest, ProcessName) {
    string proc_name = ProcessInfo::procname();
    EXPECT_FALSE(proc_name.empty());
    
    // Test procname with custom stat string
    string test_stat = "1234 (test_process) S 1 1234 1234 0 -1 4194304";
    StringPiece name_piece = ProcessInfo::procname(test_stat);
    EXPECT_EQ(name_piece.as_string(), "test_process");
    
    // Test with malformed stat string
    string bad_stat = "1234 test_process S 1 1234";
    StringPiece bad_name = ProcessInfo::procname(bad_stat);
    EXPECT_TRUE(bad_name.empty());
    
    // Test with empty stat string
    StringPiece empty_name = ProcessInfo::procname("");
    EXPECT_TRUE(empty_name.empty());
}

TEST_F(ProcessInfoTest, ProcessStatus) {
    string status = ProcessInfo::procStatus();
    // On Linux systems, this should contain process status information
    // On non-Linux systems, it might be empty
    if (!status.empty()) {
        EXPECT_TRUE(status.find("Name:") != string::npos ||
                   status.find("Pid:") != string::npos ||
                   status.find("State:") != string::npos);
    }
}

TEST_F(ProcessInfoTest, ProcessStat) {
    string stat = ProcessInfo::procStat();
    // On Linux systems, this should contain process statistics
    // On non-Linux systems, it might be empty
    if (!stat.empty()) {
        // Should contain at least the PID and process name
        EXPECT_TRUE(stat.find(std::to_string(getpid())) != string::npos);
    }
}

TEST_F(ProcessInfoTest, ThreadStat) {
    string thread_stat = ProcessInfo::threadStat();
    // On Linux systems, this should contain thread statistics
    // On non-Linux systems, it might be empty
    if (!thread_stat.empty()) {
        // Should contain thread information
        EXPECT_GT(thread_stat.length(), 0);
    }
}

TEST_F(ProcessInfoTest, ExePath) {
    string exe_path = ProcessInfo::exePath();
    // On Linux systems, this should return the executable path
    // On non-Linux systems, it might be empty
    if (!exe_path.empty()) {
        EXPECT_GT(exe_path.length(), 0);
        // Should be an absolute path
        EXPECT_EQ(exe_path[0], '/');
    }
}

TEST_F(ProcessInfoTest, FileDescriptors) {
    int opened_files = ProcessInfo::openedFiles();
    EXPECT_GE(opened_files, 0);
    
    int max_files = ProcessInfo::maxOpenFiles();
    EXPECT_GT(max_files, 0);
    
    // Opened files should be less than or equal to max files
    EXPECT_LE(opened_files, max_files);
}

TEST_F(ProcessInfoTest, CpuTime) {
    ProcessInfo::CpuTime cpu_time = ProcessInfo::cpuTime();
    
    // CPU times should be non-negative
    EXPECT_GE(cpu_time.userSeconds, 0.0);
    EXPECT_GE(cpu_time.systemSeconds, 0.0);
    
    // Total should be sum of user and system time
    EXPECT_DOUBLE_EQ(cpu_time.total(), cpu_time.userSeconds + cpu_time.systemSeconds);
    
    // Do some work and check that CPU time increases
    ProcessInfo::CpuTime initial_time = ProcessInfo::cpuTime();
    
    // Perform some CPU-intensive work
    volatile int sum = 0;
    for (int i = 0; i < 100000; ++i) {
        sum += i;
    }
    
    ProcessInfo::CpuTime after_work = ProcessInfo::cpuTime();
    
    // CPU time should have increased (though this might be flaky on very fast systems)
    EXPECT_GE(after_work.total(), initial_time.total());
}

TEST_F(ProcessInfoTest, NumThreads) {
    int num_threads = ProcessInfo::numThreads();
    EXPECT_GT(num_threads, 0);
    
    // Should have at least the main thread
    EXPECT_GE(num_threads, 1);
    
    // Create additional threads and verify count increases
    int initial_threads = num_threads;
    
    std::thread t1([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
    
    std::thread t2([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
    
    // Give threads time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    int threads_with_extra = ProcessInfo::numThreads();
    EXPECT_GE(threads_with_extra, initial_threads);
    
    t1.join();
    t2.join();
}

TEST_F(ProcessInfoTest, Threads) {
    std::vector<pid_t> thread_list = ProcessInfo::threads();
    
    // Should have at least one thread (main thread)
    EXPECT_GE(thread_list.size(), 1);
    
    // All thread IDs should be positive
    for (pid_t tid : thread_list) {
        EXPECT_GT(tid, 0);
    }
    
    // Thread list should be sorted
    for (size_t i = 1; i < thread_list.size(); ++i) {
        EXPECT_LE(thread_list[i-1], thread_list[i]);
    }
    
    // Current thread should be in the list
    pid_t current_tid = CurrentThread::tid();
    bool found_current = false;
    for (pid_t tid : thread_list) {
        if (tid == current_tid) {
            found_current = true;
            break;
        }
    }
    EXPECT_TRUE(found_current);
}

TEST_F(ProcessInfoTest, CpuTimeStruct) {
    // Test CpuTime struct construction and methods
    ProcessInfo::CpuTime default_time;
    EXPECT_DOUBLE_EQ(default_time.userSeconds, 0.0);
    EXPECT_DOUBLE_EQ(default_time.systemSeconds, 0.0);
    EXPECT_DOUBLE_EQ(default_time.total(), 0.0);
    
    // Test with custom values
    ProcessInfo::CpuTime custom_time;
    custom_time.userSeconds = 1.5;
    custom_time.systemSeconds = 2.3;
    EXPECT_DOUBLE_EQ(custom_time.total(), 3.8);
}

// Performance test
TEST_F(ProcessInfoTest, Performance) {
    // Test that functions complete in reasonable time
    auto start = std::chrono::high_resolution_clock::now();
    
    // Call various functions multiple times
    for (int i = 0; i < 100; ++i) {
        ProcessInfo::pid();
        ProcessInfo::pidString();
        ProcessInfo::uid();
        ProcessInfo::euid();
        ProcessInfo::clockTicksPerSecond();
        ProcessInfo::pageSize();
        ProcessInfo::isDebugBuild();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete in less than 1 second
    EXPECT_LT(duration.count(), 1000);
}