/*
 * @Author: WhiteCells
 * @Date: 2024-03-15 10:31:00
 * @Last Modified by: WhiteCells
 * @Last Modified time: 2024-03-15 10:31:00
 * @Description: 
*/

#include "sql_connect_pool.h"

SqlConnectPool *SqlConnectPool::instance() {
    static SqlConnectPool connect_pool;
    return &connect_pool;
}

void SqlConnectPool::init(const char *host, int port,
                          const char *user, const char *passwd,
                          const  char *db_name, int connect_size) {
    assert(connect_size > 0);
    for (int i = 0; i < connect_size; ++i) {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if (!sql) {
            LOG_ERROR("MySql init error");
            assert(sql);
        }
        sql = mysql_real_connect(sql, host,
                                 user, passwd,
                                 db_name, port, nullptr, 0);
        if (!sql) {
            LOG_ERROR("MySql connect error");
        }
        connect_que_.push(sql);
    }
    max_connect_ = connect_size;
    sem_init(&sem_id_, 0, max_connect_);
}

MYSQL *SqlConnectPool::getConnect() {
    if (connect_que_.empty()) {
        LOG_WARN("SqlConnectPool busy");
        return nullptr;
    }

    MYSQL *sql = nullptr;
    sem_wait(&sem_id_);

    {
        std::lock_guard<std::mutex> locker(mtx_);
        sql = connect_que_.front();
        connect_que_.pop();
    }

    return sql;
}

void SqlConnectPool::freeConnect(MYSQL *sql) {
    assert(sql);
    std::lock_guard<std::mutex> locker(mtx_);
    connect_que_.push(sql);
    sem_post(&sem_id_);
}

void SqlConnectPool::closePool() {
    std::lock_guard<std::mutex> locker(mtx_);
    while (!connect_que_.empty()) {
        auto item = connect_que_.front();
        connect_que_.pop();
        mysql_close(item);
    }
}

SqlConnectPool::SqlConnectPool() {
    use_count_ = 0;
    free_count_ = 0;
}

SqlConnectPool::~SqlConnectPool() {
    closePool();
}