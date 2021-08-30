#include "LogFile.h"
#include <time.h>

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
    auto pair = this->getLogFileName(basename_);
    return true;
}

std::pair<time_t, string> LogFile::getLogFileName(const string &basename, Timestamp::TimeStandard standrad)
{
    return {time(NULL), basename};
}