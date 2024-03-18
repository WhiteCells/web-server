/*
 * @Author: Author
 * @Date: 2024-03-16 17:37:46
 * @Last Modified by: Author
 * @Last Modified time: 2024-03-16 17:37:46
 * @Description:
*/

#include "epoller.h"

Epoller::Epoller(int max_events) :
    epoll_fd_(epoll_create(512)), events_(max_events) {
    assert(epoll_fd_ >= 0 && events_.size() > 0);
}

Epoller::~Epoller() {
    close(epoll_fd_);
}

bool Epoller::addFd(int fd, uint32_t events) {
    if (fd < 0) {
        return false;
    }
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::modFd(int fd, uint32_t events) {
    if (fd < 0) {
        return false;
    }
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
}

bool Epoller::delFd(int fd) {
    if (fd < 0) {
        return false;
    }
    epoll_event ev = {0};
    return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &ev);
}

int Epoller::wait(int timeout_ms) {
    return epoll_wait(epoll_fd_, &events_[0],
                      static_cast<int>(events_.size()), timeout_ms);
}

int Epoller::getEventFd(size_t i) const {
    assert(i >= 0 && i < events_.size());
    return events_[i].data.fd;
}

uint32_t Epoller::getEvents(size_t i) const {
    assert(i >= 0 && i < events_.size());
    return events_[i].events;
}