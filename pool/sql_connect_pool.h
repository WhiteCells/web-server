/*
 * @Author: WhiteCells
 * @Date: 2024-03-11 09:06:25
 * @Last Modified by: WhiteCells
 * @Last Modified time: 2024-03-11 09:06:25
 * @Description: 
*/

#ifndef _SQLCONNPOOL_H_
#define _SQLCONNPOOL_H_

#include "../log/log.h"
#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>

class SqlConnectPool {
public:
    static SqlConnectPool *instance();

    void init(const char *host, int port,
              const char *user, const char *passwd,
              const char *db_name, int connect_size = 10);
    MYSQL *getConnect();
    void freeConnect(MYSQL *connect);
    int getFreeConnectCount();
    void closePool();

private:
    SqlConnectPool();
    ~SqlConnectPool();

    int max_connect_;
    int use_count_;
    int free_count_;

    std::queue<MYSQL *> connect_que_;
    std::mutex mtx_;
    sem_t sem_id_;
};

#endif // _SQLCONNPOOL_H_