/*
 * @Author: Author
 * @Date: 2024-03-16 17:57:14
 * @Last Modified by: Author
 * @Last Modified time: 2024-03-16 17:57:14
 * @Description:
*/

#ifndef _WEB_SERVER_H_
#define _WEB_SERVER_H_

#include "epoller.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/sql_connect_pool.h"
#include "../pool/sql_connect_raii.h"
#include "../pool/thread_pool.h"
#include "../http/http_connect.h"

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class WebServer {
public:
    WebServer(
        int port, int trig_mode, int timeout_ms, bool opt_linger,
        int sql_port, const char *sql_user, const char *sql_pwd,
        const char *db_name, int connect_pool_num, int thread_num,
        bool open_log, int log_level, int log_que_size
    );
    ~WebServer();

    void start();

private:
    bool initSocket_();
    void initEventMode_(int trig_mode);
    void addClient_(int fd, sockaddr_in addr);

    void dealListen_();
    void dealWrite_(HttpConnect *client);
    void dealRead_(HttpConnect *client);

    void sendError_(int fd, const char *info);
    void extentTime_(HttpConnect *client);
    void closeConnect_(HttpConnect *client);

    void onRead_(HttpConnect *client);
    void onWrite_(HttpConnect *client);
    void onProcess_(HttpConnect *client);

    static int setFdNonblock(int fd);

    static const int MAX_FD = 65536;

    int port_;
    bool open_linger_;
    int timeout_ms_;
    bool is_close_;
    int listen_fd_;
    char *src_dir_;

    uint32_t listen_event_;
    uint32_t connect_event_;

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConnect> users_;
};

#endif // _WEB_SERVER_H_