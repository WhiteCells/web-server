/*
 * @Author: Author
 * @Date: 2024-03-16 18:59:00
 * @Last Modified by: Author
 * @Last Modified time: 2024-03-16 18:59:00
 * @Description:
*/

#include "web_server.h"

WebServer::WebServer(
    int port, int trig_mode, int timeout_ms, bool opt_linger,
    int sql_port, const char *sql_user, const char *sql_pwd,
    const char *db_name, int connect_pool_num, int thread_num,
    bool open_log, int log_level, int log_que_size
) : port_(port), open_linger_(opt_linger),
timeout_ms_(timeout_ms), is_close_(false),
timer_(new HeapTimer()), threadpool_(new ThreadPool(thread_num)),
epoller_(new Epoller()) {
    
    src_dir_ = getcwd(nullptr, 256);
    assert(src_dir_);
    strncat(src_dir_, "/resources/", 16);
    HttpConnect::user_count = 0;
    HttpConnect::src_dir = src_dir_;
    SqlConnectPool::instance()->init("localhost", sql_port,
                                     sql_user, sql_pwd,
                                     db_name, connect_pool_num);

    initEventMode_(trig_mode);
    if (!initSocket_()) {
        is_close_ = true;
    }

    if (open_log) {
        Log::instance()->init(log_level, "./log/", ".log",
                              log_que_size);
        if (is_close_) {
            LOG_ERROR("### Server init error ###");
        } else {
            LOG_INFO("Port: %d, OpenLinger: %s", port_, opt_linger ? "true" : "false");
            LOG_INFO("Listen Mode: %s, OpenConnect Mode: %s",
                     ((listen_event_ & EPOLLET) ? "ET" : "LT"),
                     ((connect_event_ & EPOLLET) ? "ET" : "LT"));
            LOG_INFO("Log level: %d", log_level);
            LOG_INFO("src dir: %s", HttpConnect::src_dir);
            LOG_INFO("SqlConnectPool num: %d, ThreadPool num: %d",
                     connect_pool_num, thread_num);
        }
    }
}

WebServer::~WebServer() {
    close(listen_fd_);
    is_close_ = true;
    free(src_dir_);
    SqlConnectPool::instance()->closePool();
}

void WebServer::start() {
    int time_ms = -1;
    if (!is_close_) {
        LOG_INFO("### Server start ###");
    }
    while (!is_close_) {
        if (timeout_ms_ > 0) {
            time_ms = timer_->getNextTick();
        }
        int event_count = epoller_->wait(time_ms);
        for (int i = 0; i < event_count; ++i) {
            int fd = epoller_->getEventFd(i);
            uint32_t events = epoller_->getEvents(i);
            if (fd == listen_fd_) {
                dealListen_();
            } else if (events && (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                closeConnect_(&users_[fd]);
            } else if (events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                dealRead_(&users_[fd]);
            } else if (events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                dealWrite_(&users_[fd]);
            } else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

bool WebServer::initSocket_() {
    struct sockaddr_in addr;
    if (port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Port: %d error", port_);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    struct linger opt_linger = {0};
    if (open_linger_) {
        opt_linger.l_onoff = 1;
        opt_linger.l_linger = 1;
    }

    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        LOG_ERROR("Create socket error", port_);
        return false;
    }

    int ret = setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER,
                     &opt_linger, sizeof(opt_linger));
    if (ret < 0) {
        LOG_ERROR("init linger error", port_);
        close(listen_fd_);
        return false;
    }

    int optval = 1;
    ret = setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER,
                     &opt_linger, sizeof(opt_linger));
    if (ret == -1) {
        LOG_ERROR("set socket error");
        close(listen_fd_);
        return false;
    }

    ret = bind(listen_fd_, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        LOG_ERROR("Bind Port: %d error", port_);
        close(listen_fd_);
        return false;
    }

    ret = listen(listen_fd_, 6);
    if (ret < 0) {
        LOG_ERROR("Listen Port: %d error", port_);
        close(listen_fd_);
        return false;
    }

    ret = epoller_->addFd(listen_fd_, listen_event_ | EPOLLIN);
    if (ret == 0) {
        LOG_ERROR("add listent error");
        close(listen_fd_);
        return false;
    }

    ret = setFdNonblock(listen_fd_);
    if (ret == -1) {
        LOG_ERROR("set fd nonblock error");
        return false;
    }
    LOG_INFO("Server Port: %d", port_);
    return true;
}

void WebServer::initEventMode_(int trig_mode) {
    listen_event_ = EPOLLRDHUP;
    connect_event_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trig_mode) {
        case 0:
            break;
        case 1:
            connect_event_ |= EPOLLET;
            break;
        case 2:
            listen_event_ |= EPOLLET;
            break;
        case 3:
            listen_event_ |= EPOLLET;
            connect_event_ |= EPOLLET;
            break;
        default:
            listen_event_ |= EPOLLET;
            connect_event_ |= EPOLLET;
            break;
    }
    HttpConnect::is_ET = (connect_event_ & EPOLLET);
}

void WebServer::addClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].init(fd, addr);
    if (timeout_ms_ > 0) {
        timer_->add(fd, timeout_ms_, std::bind(&WebServer::closeConnect_,
                                               this, &users_[fd]));
    }
    epoller_->addFd(fd, EPOLLIN | connect_event_);
    setFdNonblock(fd);
    LOG_INFO("Client[%d] in", users_[fd].getFd());
}

void WebServer::dealListen_() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {

    } while (listen_event_ & EPOLLET);
}

void WebServer::dealWrite_(HttpConnect *client) {

}

void WebServer::dealRead_(HttpConnect *client) {

}

void WebServer::sendError_(int fd, const char *info) {

}

void WebServer::extentTime_(HttpConnect *client) {

}

void WebServer::closeConnect_(HttpConnect *client) {

}

void WebServer::onRead_(HttpConnect *client) {

}

void WebServer::onWrite_(HttpConnect *client) {

}

void WebServer::onProcess_(HttpConnect *client) {

}

int WebServer::setFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}