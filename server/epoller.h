/*
 * @Author: Author
 * @Date: 2024-03-16 17:37:52
 * @Last Modified by: Author
 * @Last Modified time: 2024-03-16 17:37:52
 * @Description: 
*/

#ifndef _EPOLLER_H_
#define _EPOLLER_H_

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <vector>

class Epoller {
public:
    explicit Epoller(int max_event = 1024);
    ~Epoller();

    bool addFd(int fd, uint32_t events);
    bool modFd(int fd, uint32_t events);
    bool delFd(int fd);
    int wait(int timeout_ms = -1);

    int getEventFd(size_t i) const;
    uint32_t getEvents(size_t i) const;

private:
    int epoll_fd_;
    std::vector<struct epoll_event> events_;
};

#endif // _EPOLLER_H_