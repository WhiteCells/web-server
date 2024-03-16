/*
 * @Author: WhiteCells
 * @Date: 2024-03-08 12:00:47
 * @Last Modified by: WhiteCells
 * @Last Modified time: 2024-03-08 12:00:47
 * @Description: 
*/

#include "buffer.h"

Buffer::Buffer(int init_buffer_size) :
    buffer_(init_buffer_size),
    read_pos_(0),
    write_pos_(0) {
    
}

const char *Buffer::peek() const {
    return beginPtr_() + read_pos_;
}

size_t Buffer::getWriteableBytes() const {
    return buffer_.size() - write_pos_;
}

size_t Buffer::getReadableBytes() const {
    return write_pos_ - read_pos_;
}

size_t Buffer::getPrependableBytes() const {
    return read_pos_;
}

void Buffer::ensureWriteable(size_t len) {
    if (getWriteableBytes() < len) {
        makeSpace_(len);
    }
    assert(getWriteableBytes() >= len);
}

void Buffer::hasWritten(size_t len) {
    write_pos_ += len;
}

void Buffer::retrieve(size_t len) {
    assert(len <= getReadableBytes());
    read_pos_ += len;
}

void Buffer::retrieveUntil(const char *end) {
    assert(peek() <= end);
    retrieve(end - peek());
}

void Buffer::retrieveAll() {
    bzero(&buffer_[0], buffer_.size());
    read_pos_ = 0;
    write_pos_ = 0;
}

std::string Buffer::retrieveAllToStr() {
    std::string str(peek(), getReadableBytes());
    retrieveAll();
    return str;
}

const char *Buffer::beginWriteConst() const {
    return beginPtr_() + write_pos_;
}

char *Buffer::beginWrite() {
    return beginPtr_() + write_pos_;
}

void Buffer::append(const std::string &str) {
    append(str.data(), str.length());
}

void Buffer::append(const char *str, size_t len) {
    assert(str);
    ensureWriteable(len);
    std::copy(str, str + len, beginWrite());
    hasWritten(len);
}

void Buffer::append(const void *data, size_t len) {
    assert(data);
    append(static_cast<const char *>(data), len);
}

void Buffer::append(const Buffer &buff) {
    append(buff.peek(), buff.getReadableBytes());
}

ssize_t Buffer::readFd(int fd, int *save_errno) {
    char buff[65535];
    struct iovec iov[2];
    const size_t writeable = getWriteableBytes();

    iov[0].iov_base = beginPtr_() + write_pos_;
    iov[0].iov_len = writeable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *save_errno = errno;
    } else if (static_cast<size_t>(len) <= writeable) {
        write_pos_ += len;
    } else {
        write_pos_ = buffer_.size();
        append(buff, len - writeable);
    }

    return len;
}

ssize_t Buffer::writeFd(int fd, int *save_errno) {
    size_t read_size = getReadableBytes();
    ssize_t len = write(fd, peek(), read_size);

    if (len < 0) {
        *save_errno = errno;
        return len;
    }

    read_pos_ += len;
    return len;
}

char *Buffer::beginPtr_() {
    return &*buffer_.begin();
}

const char *Buffer::beginPtr_() const {
    return &*buffer_.begin();
}

void Buffer::makeSpace_(size_t len) {
    if (getWriteableBytes() + getPrependableBytes() < len) {
        buffer_.resize(write_pos_ + len + 1);
    } else {
        size_t readable = getReadableBytes();
        std::copy(beginPtr_() + read_pos_,
                  beginPtr_() + write_pos_, beginPtr_());
        read_pos_ = 0;
        write_pos_ = read_pos_ + readable;
        assert(readable == getReadableBytes());
    }
}