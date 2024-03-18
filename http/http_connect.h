/*
 * @Author: WhiteCells
 * @Date: 2024-03-15 10:30:39
 * @Last Modified by: WhiteCells
 * @Last Modified time: 2024-03-15 10:30:39
 * @Description: 
*/

#ifndef _HTTP_CONNECT_H_
#define _HTTP_CONNECT_H_

#include "../log/log.h"
#include "../pool/sql_connect_pool.h"
#include "../buffer/buffer.h"
#include "http_request.h"
#include "http_response.h"

#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <error.h>

class HttpConnect {
public:
    HttpConnect();
    ~HttpConnect();

    void init(int sock_fd, const sockaddr_in &addr);
    void closeConnect();

    ssize_t read(int *save_errno);
    ssize_t write(int *save_errno);

    int getFd() const;
    const char *getIP() const;
    int getPort() const;
    sockaddr_in getAddr() const;
    bool process();
    int writeBytes();
    bool isKeepAlive() const;

    static bool is_ET;
    static const char *src_dir;
    static std::atomic<int> user_count;

private:
    int fd_;
    struct sockaddr_in addr_;
    bool is_close_;
    int iov_count_;
    struct iovec iov_[2];

    Buffer read_buff_;
    Buffer write_buff_;

    HttpRequest request_;
    HttpResponse response_;
};


#endif // _HTTP_CONNECT_H_