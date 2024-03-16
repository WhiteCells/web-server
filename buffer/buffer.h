/*
 * @Author: WhiteCells
 * @Date: 2024-03-07 19:56:54
 * @Last Modified by: WhiteCells
 * @Last Modified time: 2024-03-07 19:56:54
 * @Description: 
*/

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <cstring>
#include <cassert>
#include <iostream>
#include <vector>
#include <atomic>
#include <unistd.h>
#include <sys/uio.h>

class Buffer {
public:
    Buffer(int init_buffer_size = 1024);
    ~Buffer() = default;

    const char *peek() const;

    size_t getWriteableBytes() const;
    size_t getReadableBytes() const;
    size_t getPrependableBytes() const;

    void ensureWriteable(size_t len);
    void hasWritten(size_t len);

    void retrieve(size_t len);
    void retrieveUntil(const char *end);
    void retrieveAll();
    std::string retrieveAllToStr();

    const char *beginWriteConst() const;
    char *beginWrite();

    void append(const std::string &str);
    void append(const char *str, size_t len);
    void append(const void *data, size_t len);
    void append(const Buffer &buff);

    ssize_t readFd(int fd, int *save_errno);
    ssize_t writeFd(int fd, int *save_errno);

private:
    char *beginPtr_();
    const char *beginPtr_() const;
    void makeSpace_(size_t len);

    std::vector<char> buffer_;
    std::atomic<std::size_t> read_pos_;
    std::atomic<std::size_t> write_pos_;
};

#endif // _BUFFER_H_