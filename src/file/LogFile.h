/**
 * File object that supports for logging writing, 
 * so that log stream can use it to write into file system instead printing to console.
 * The log file class is NOT thread-safe, you will need another class to protect its safety.
 * In this project, that is Hornor::AsyncLogging
 * 
 * The logging file class has 3 functionalities:
 * 1. Append buffer message to the file.
 * 2. Automatically check and flush cached message in memory into the hard drive.
 * 3. Automatically check and roll new log files, 
 * which means to close the current writing file and start a new one, 
 * in this way the log files would not be too huge and easier to manage.
 */

#pragma once
#include "NonCopyable.h"
#include "Timestamp.h"
#include "FileUtils.h"
#include <map>
#include <memory>

namespace Hohnor
{
    class LogFile : Hohnor::NonCopyable
    {
    public:
        //Initilize with file base name and directory
        explicit LogFile(const string &basename,
                         const string &directory = "./",
                         int checkEveryN = 1024,
                         int flushInterval = 3,
                         off_t rollSize = 65536 * 256, //This is 4 MBytes space by default
                         int rollInterval = 60 * 60 * 24,
                         Timestamp::TimeStandard standrad = Timestamp::UTC);
        ~LogFile();

        void append(const char *logline, int len);
        //Manually flush into hard drive
        void flush();
        //Manually roll a new file
        void rollFile();

    private:
        //use a base name comibined with time infor to create new log file name
        static std::pair<time_t, string> getLogFileName(const string &basename, const string &directory = "./", Timestamp::TimeStandard standrad = Timestamp::UTC);
        //Store basename passed from the contructor
        const string basename_;
        //Store director
        const string directory_;
        //Every N writes, the class will go to check if should run roll and flush function
        const int checkEveryN_;
        //Maximum size that the class will roll a new file, passed in the contructor
        const off_t rollSize_;
        //interval of time between flushing file message in memory into hard drive
        const int flushInterval_;
        //seconds to roll
        const int rollInterval_;
        //Time standard, could be UTC or GMT
        const Timestamp::TimeStandard standrad_;

        //Counting number of writes
        int count_;

        time_t lastRoll_;
        time_t lastFlush_;
        std::unique_ptr<FileUtils::AppendFile> file_;
    };
} // namespace Hohnor
