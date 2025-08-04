#include "hohnor/file/LogFile.h"
#include <time.h>
#include "hohnor/time/Timestamp.h"

using namespace Hohnor;
LogFile::LogFile(const string &basename,
                 const string &directory,
                 int checkEveryN,
                 int flushInterval,
                 off_t rollSize,
                 int rollInterval,
                 Timestamp::TimeStandard standrad)
    : basename_(basename),
      directory_(directory),
      checkEveryN_(checkEveryN),
      flushInterval_(flushInterval),
      rollSize_(rollSize),
      rollInterval_(rollInterval),
      count_(0),
      lastRoll_(time(NULL)),
      lastFlush_(time(NULL)),
      standrad_(standrad)
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

void LogFile::rollFile()
{
    auto pair = this->getLogFileName(basename_, directory_, standrad_);
    lastRoll_ = pair.first;
    lastFlush_ = pair.first;
    file_.reset(new FileUtils::AppendFile(pair.second));
}

void LogFile::flush()
{
    lastFlush_ = time(NULL);
    file_->flush();
}

std::pair<time_t, string> LogFile::getLogFileName(const string &basename, const string &dir, Timestamp::TimeStandard standard)
{
    string res = dir;
    if (res[res.length() - 1] != '/')
        res += '/';
    res += basename;
    char timebuf[32];
    struct tm tm;
    time_t now = time(NULL);
    if (standard == Timestamp::GMT)
        gmtime_r(&now, &tm);
    else
        localtime_r(&now, &tm);
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
    res += timebuf;
    res += "log";
    return {now, res};
}