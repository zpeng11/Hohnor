#include "FileUtils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

using namespace Hohnor;
using namespace Hohnor::FileUtils;

//Use thread safe strerror call to safe erro number information
std::string strerror_tl(int savedErrno)
{
	char t_errnobuf[512];
#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !_GNU_SOURCE
	strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
	return t_errnobuf;
#else
	return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
#endif
}

AppendFile::AppendFile(StringPiece filename)
{
	fp_ = fopen(filename.data(), "ae+"); //e for  O_CLOEXEC (since Linux 2.6.23)
	assert(fp_ != NULL);
	writtenBytes_ = 0;
	setbuffer(fp_, buffer_, sizeof buffer_);
}

AppendFile::~AppendFile()
{
	fclose(fp_);
}

void AppendFile::append(const char *logline, const size_t len)
{
	size_t written = 0;

	while (written != len)
	{
		size_t remain = len - written;
		size_t n = write(logline + written, remain);
		if (n != remain)
		{
			int err = ferror(fp_);
			if (err)
			{
				perror(strerror_tl(err).c_str());
				break;
			}
		}
		written += n;
	}

	writtenBytes_ += written;
}

void AppendFile::flush()
{
	fflush(fp_);
}

size_t AppendFile::write(const char *logline, size_t len)
{
	// #undef fwrite_unlocked
	return fwrite_unlocked(logline, 1, len, fp_);
}

ReadSmallFile::ReadSmallFile(StringPiece filename)
	: fd_(open(filename.data(), O_RDONLY | O_CLOEXEC)),
	  err_(0)
{
	buf_[0] = '\0';
	if (fd_ < 0)
	{
		err_ = errno;
	}
}

ReadSmallFile::~ReadSmallFile()
{
	if (fd_ >= 0)
	{
		close(fd_); // FIXME: check EINTR
	}
}

// return errno
template <typename String>
int ReadSmallFile::readToString(int maxSize,
								String *content,
								int64_t *fileSize,
								int64_t *modifyTime,
								int64_t *createTime)
{
	static_assert(sizeof(off_t) == 8, "_FILE_OFFSET_BITS = 64");
	assert(content != NULL);
	int err = err_;
	if (fd_ >= 0)
	{
		content->clear();

		if (fileSize)
		{
			struct stat statbuf;
			if (fstat(fd_, &statbuf) == 0)
			{
				if (S_ISREG(statbuf.st_mode))
				{
					*fileSize = statbuf.st_size;
					content->reserve(static_cast<int>(std::min(implicit_cast<int64_t>(maxSize), *fileSize)));
				}
				else if (S_ISDIR(statbuf.st_mode))
				{
					err = EISDIR;
				}
				if (modifyTime)
				{
					*modifyTime = statbuf.st_mtime;
				}
				if (createTime)
				{
					*createTime = statbuf.st_ctime;
				}
			}
			else
			{
				err = errno;
			}
		}

		while (content->size() < implicit_cast<size_t>(maxSize))
		{
			size_t toRead = std::min(implicit_cast<size_t>(maxSize) - content->size(), sizeof(buf_));
			ssize_t n = read(fd_, buf_, toRead);
			if (n > 0)
			{
				content->append(buf_, n);
			}
			else
			{
				if (n < 0)
				{
					err = errno;
				}
				break;
			}
		}
	}
	return err;
}

int ReadSmallFile::readToBuffer(int *size)
{
	int err = err_;
	if (fd_ >= 0)
	{
		ssize_t n = pread(fd_, buf_, sizeof(buf_) - 1, 0);
		if (n >= 0)
		{
			if (size)
			{
				*size = static_cast<int>(n);
			}
			buf_[n] = '\0';
		}
		else
		{
			err = errno;
		}
	}
	return err;
}

template int readFile(StringPiece filename,
					  int maxSize,
					  string *content,
					  int64_t *, int64_t *, int64_t *);

template int ReadSmallFile::readToString(
	int maxSize,
	string *content,
	int64_t *, int64_t *, int64_t *);