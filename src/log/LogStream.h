/**
 * Lower level stream object that saves text information, we implement our own buffer and own stream,
 * since the std::basic_buffer and std::ostream are too heavy and hard to understand and use(unreadable)
 */
#pragma once
#include "Types.h"
#include "NonCopyable.h"
#include "StringPiece.h"
#include <iostream>
#include <string.h>
#include <memory>

namespace Hohnor
{
	namespace LogBuffer
	{
		const int kSmallBuffer = 4000;
		const int kLargeBuffer = 4000 * 1000;

		template <int SIZE>
		class FixedBuffer : Hohnor::NonCopyable
		{
		public:
			FixedBuffer()
				: cur_(data_)
			{
				setCookie(cookieStart);
			}

			~FixedBuffer()
			{
				setCookie(cookieEnd);
			}

			bool append(const char * /*restrict*/ buf, size_t len)
			{
				if (LIKELY(implicit_cast<size_t>(avail()) > len))
				{
					memcpy(cur_, buf, len);
					cur_ += len;
					return true;
				}
				else
				{
					return false;
				}
			}

			const char *data() const { return data_; }
			int length() const { return static_cast<int>(cur_ - data_); }

			// write to data_ directly
			char *current() { return cur_; }
			int avail() const { return static_cast<int>(end() - cur_); }
			void add(size_t len) { cur_ += len; }

			void reset() { cur_ = data_; }
			void bzero() { memZero(data_, sizeof data_); }

			// for used by GDB
			const char *debugString()
			{
				*cur_ = '\0';
				return data_;
			};
			void setCookie(void (*cookie)()) { cookie_ = cookie; }
			// for used by unit test
			string toString() const { return string(data_, length()); }
			StringPiece toStringPiece() const { return StringPiece(data_, length()); }

		private:
			const char *end() const { return data_ + sizeof data_; }
			// Must be outline function for cookies.
			static void cookieStart();
			static void cookieEnd();

			void (*cookie_)();
			char data_[SIZE];
			char *cur_;
		};
	}

	class LogStream : NonCopyable
	{
		typedef LogStream self;

	public:
		typedef LogBuffer::FixedBuffer<LogBuffer::kSmallBuffer> Buffer;

		LogStream() : buffer_(new LogStream::Buffer()) {}
		self &operator<<(bool v)
		{
			if (v)
			{
				buffer_->append("true", 4);
			}
			else
			{
				buffer_->append("false", 5);
			}
			return *this;
		}

		self &operator<<(float v)
		{
			*this << static_cast<double>(v);
			return *this;
		}

		self &operator<<(double v)
		{
			if (buffer_->avail() >= kMaxNumericSize)
			{
				int len = snprintf(buffer_->current(), kMaxNumericSize, "%.12g", v);
				buffer_->add(len);
			}
			return *this;
		};

		self &operator<<(char v)
		{
			buffer_->append(&v, 1);
			return *this;
		}

		// self& operator<<(signed char);
		// self& operator<<(unsigned char);

		self &operator<<(const char *str)
		{
			if (str)
			{
				buffer_->append(str, strlen(str));
			}
			else
			{
				buffer_->append("(null)", 6);
			}
			return *this;
		}

		self &operator<<(const unsigned char *str)
		{
			return operator<<(reinterpret_cast<const char *>(str));
		}

		self &operator<<(const string &v)
		{
			buffer_->append(v.c_str(), v.size());
			return *this;
		}

		self &operator<<(const StringPiece &v)
		{
			buffer_->append(v.data(), v.size());
			return *this;
		}

		self &operator<<(const Buffer &v)
		{
			*this << v.toStringPiece();
			return *this;
		}

		self &operator<<(short);
		self &operator<<(unsigned short);
		self &operator<<(int);
		self &operator<<(unsigned int);
		self &operator<<(long);
		self &operator<<(unsigned long);
		self &operator<<(long long);
		self &operator<<(unsigned long long);

		self &operator<<(const void *);

		void append(const char *data, int len) { buffer_->append(data, len); }
		const Buffer &buffer() const { return *buffer_; }
		void resetBuffer() { buffer_->reset(); }
		std::shared_ptr<Buffer> moveBuffer() { return std::move(buffer_); }

	private:
		void staticCheck();

		template <typename T>
		void formatInteger(T);

		std::shared_ptr<LogStream::Buffer> buffer_;

		static const int kMaxNumericSize = 48;
	};

	class Fmt // : noncopyable
	{
	public:
		template <typename T>
		Fmt(const char *fmt, T val);

		const char *data() const { return buf_; }
		int length() const { return length_; }

	private:
		char buf_[32] = {0};
		int length_;
	};

	inline LogStream &operator<<(LogStream &s, const Fmt &fmt)
	{
		s.append(fmt.data(), fmt.length());
		return s;
	}

    inline std::ostream &operator<<(std::ostream &s, const Fmt &fmt)
	{
        
		s.write(fmt.data(), fmt.length());
		return s;
	}

    inline string& operator+=(std::string &s, const Fmt &fmt)
    {
        s.append(fmt.data(), fmt.length());
        return s;
    }

	// Format quantity n in SI units (k, M, G, T, P, E).
	// The returned string is atmost 5 characters long.
	// Requires n >= 0
	string formatSI(int64_t n);

	// Format quantity n in IEC (binary) units (Ki, Mi, Gi, Ti, Pi, Ei).
	// The returned string is atmost 6 characters long.
	// Requires n >= 0
	string formatIEC(int64_t n);

}