#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "hohnor/common/StringPiece.h"

namespace Hohnor {
    constexpr size_t kCheapPrepend = 8;
    constexpr size_t kInitialSize = 1024;
/// A buffer class modeled after Netty's ByteBuf, improved with StringPiece.
///
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex  <=      writerIndex     <=     size
class Buffer {
public:
    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend) {}

    // --- Capacity and Index Info ---
    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }
    size_t prependableBytes() const { return readerIndex_; }
    size_t capacity() const { return buffer_.capacity(); }

    // --- Read Operations ---

    // Returns a raw pointer to the readable data.
    // Useful for passing to system calls like write().
    const char* peek() const { return begin() + readerIndex_; }

    // Returns a StringPiece view of all readable data.
    // Efficient for parsing, no copy is made.
    StringPiece readableSlice() const {
        return StringPiece(peek(), static_cast<int>(readableBytes()));
    }

    void retrieve(size_t len) {
        assert(len <= readableBytes());
        if (len < readableBytes()) {
            readerIndex_ += len;
        } else {
            retrieveAll();
        }
    }

    // Retrieves 'len' bytes as a StringPiece and advances the reader index.
    StringPiece retrieveAsSlice(size_t len) {
        assert(len <= readableBytes());
        StringPiece result(peek(), static_cast<int>(len));
        retrieve(len);
        return result;
    }

    void retrieveAll() {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len) {
        assert(len <= readableBytes());
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    const char* findCRLF() const
    {
        const char kCRLF[] = "\r\n";
        const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    const char* findCRLF(const char* start) const
    {
        assert(peek() <= start);
        const char kCRLF[] = "\r\n";
        assert(start <= beginWrite());
        const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    const char* findEOL() const
    {
        const void* eol = memchr(peek(), '\n', readableBytes());
        return static_cast<const char*>(eol);
    }

    const char* findEOL(const char* start) const
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const void* eol = memchr(start, '\n', beginWrite() - start);
        return static_cast<const char*>(eol);
    }


    // --- Append/Write Operations ---
    void append(const StringPiece& str) {
        append(str.data(), str.size());
    }

    void append(const char* data, size_t len) {
        ensureWritable(len);
        std::copy(data, data + len, beginWrite());
        hasWritten(len);
    }

    void append(const void* /*restrict*/ data, size_t len)
    {
        append(static_cast<const char*>(data), len);
    }

    void append(const std::string& str) {
        append(str.data(), str.length());
    }

    // --- Prepend Operations ---
    void prepend(const void* data, size_t len) {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char* d = static_cast<const char*>(data);
        std::copy(d, d + len, begin() + readerIndex_);
    }
    
    void prepend(const StringPiece& str) {
        prepend(str.data(), str.size());
    }

    void prepend(const std::string& str) {
        prepend(str.data(), str.length());
    }

    // --- Write Interface for Direct Access ---
    char* beginWrite() { return begin() + writerIndex_; }
    const char* beginWrite() const { return begin() + writerIndex_; }
    
    void hasWritten(size_t len) {
        assert(len <= writableBytes());
        writerIndex_ += len;
    }
    
    void ensureWritable(size_t len) {
        if (writableBytes() < len) {
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }

    void unwrite(size_t len)
    {
        assert(len <= readableBytes());
        writerIndex_ -= len;
    }

    ssize_t readFd(int fd, int* savedErrno);

private:
    char* begin() { return &*buffer_.begin(); }
    const char* begin() const { return &*buffer_.begin(); }

    void makeSpace(size_t len) {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
            // Grow the buffer
            buffer_.resize(writerIndex_ + len);
        } else {
            // Move readable data to make space, preserve cheap prepend
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writerIndex_,
                     begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
            assert(readable == readableBytes());
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};

} // namespace