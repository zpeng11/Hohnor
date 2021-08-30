#include "LogFile.h"
#include <time.h>
#include "Timestamp.h"

using namespace Hohnor;
LogFile::LogFile(const string &basename,
                 const string &directory,
                 off_t rollSize,
                 int flushInterval,
                 int checkEveryN,
                 int rollInterval)
    : basename_(basename),
      directory_(directory),
      rollSize_(rollSize),
      flushInterval_(flushInterval),
      checkEveryN_(checkEveryN),
      count_(0),
      lastRoll_(time(NULL)),
      lastFlush_(time(NULL)),
      rollInterval_(rollInterval)
{
    assert(basename.find('/') == string::npos);
    rollFile();
}

LogFile::~LogFile() = default;

void LogFile::append(const char *logline, int len)
{
    file_->append(logline, len);
    if (file_->writtenBytes() > rollSize_)
    {
        rollFile();
    }
    else
    {
        ++count_;
        if (count_ >= checkEveryN_)
        {
            count_ = 0;
            time_t now = ::time(NULL);
            if (now - lastRoll_ > rollInterval_)
            {
                rollFile();
            }
            else if (now - lastFlush_ > flushInterval_)
            {
                flush();
            }
        }
    }
}

bool LogFile::rollFile()
{
    auto pair = this->getLogFileName(basename_, directory_);
    return true;
}
#include <iostream>
std::pair<time_t, string> LogFile::getLogFileName(const string &basename, const string &dir, Timestamp::TimeStandard standard)
{
    string res = dir;
    if (res[res.length()-1] != '/')
        res += '/';
    res += basename;
    char timebuf[32];
    struct tm tm;
    time_t now = time(NULL);
    if(standard == Timestamp::GMT)
        gmtime_r(&now, &tm); // FIXME: localtime_r ?
    else
        localtime_r(&now, &tm);
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
    res += timebuf;
    res += "log";
    std::cout<<dir<<std::endl;
    std::cout<<basename<<std::endl;
    std::cout<<res<<std::endl;
    return {now, res};
}