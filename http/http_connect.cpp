/*
 * @Author: WhiteCells
 * @Date: 2024-03-15 10:30:44
 * @Last Modified by: WhiteCells
 * @Last Modified time: 2024-03-15 10:30:44
 * @Description:
*/

#include "http_connect.h"

HttpConnect::HttpConnect() {
    fd_ = -1;
    addr_ = {0};
    is_close_ = true;
}

HttpConnect::~HttpConnect() {
    closeConnect();
}

void HttpConnect::init(int sock_fd, const sockaddr_in &addr) {
    assert(sock_fd > 0);
    ++user_count;
    addr_ = addr;
    fd_ = sock_fd;
    write_buff_.retrieveAll();
    read_buff_.retrieveAll();
    is_close_ = false;
    LOG_INFO("Client[%d](%s:%d) in, user count: %d",
             fd_, getIP(), getPort(), (int)user_count);
}

void HttpConnect::closeConnect() {
    response_.unmapFile();
    if (is_close_ == false) {
        is_close_ = true;
        --user_count;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, user count: %d",
                 fd_, getIP(), getPort(), (int)user_count);
    }
}

ssize_t HttpConnect::read(int *save_errno) {
    ssize_t len = -1;
    do {
        len = read_buff_.readFd(fd_, save_errno);
        if (len <= 0) {
            break;
        }
    } while (is_ET);
    return len;
}

ssize_t HttpConnect::write(int *save_errno) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iov_count_);
        if (len <= 0) {
            *save_errno = errno;
            break;
        }
        if (iov_[0].iov_len + iov_[1].iov_len == 0) {
            break;
        } else if (static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base = (uint8_t *)iov_[1].iov_base
                + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if (iov_[0].iov_len) {
                write_buff_.retrieveAll();
                iov_[0].iov_len = 0;
            }
        } else {

        }

    } while (is_ET || writeBytes() > 10240);
    return len;
}

const char *HttpConnect::getIP() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConnect::getPort() const {
    return addr_.sin_port;
}

int HttpConnect::getFd() const {
    return fd_;
}

sockaddr_in HttpConnect::getAddr() const {
    return addr_;
}

bool HttpConnect::process() {
    request_.init();
    if (read_buff_.getReadableBytes() <= 0) {
        return false;
    } else if (request_.parse(read_buff_)) {
        LOG_DEBUG("%s", request_.path().c_str());
        response_.init(src_dir, request_.path(),
                       request_.isKeepAlive(), 200);
    } else {
        response_.init(src_dir, request_.path(),
                       false, 400);
    }

    response_.makeResponse(write_buff_);

    iov_[0].iov_base = const_cast<char *>(write_buff_.peek());
    iov_[0].iov_len = write_buff_.getReadableBytes();
    iov_count_ = 1;

    if (response_.fileLen() > 0 && response_.file()) {
        iov_[1].iov_base = response_.file();
        iov_[1].iov_len = response_.fileLen();
        iov_count_ = 2;
    }

    LOG_DEBUG("filesize: %d, %d to %d", response_.fileLen(),
              iov_count_, writeBytes());
    return true;
}

int HttpConnect::writeBytes() {
    return iov_[0].iov_len + iov_[1].iov_len;
}

bool HttpConnect::iskeepAlive() const {
    return request_.isKeepAlive();
}