/*
 * @Author: WhiteCells
 * @Date: 2024-03-15 10:30:28
 * @Last Modified by: WhiteCells
 * @Last Modified time: 2024-03-15 10:30:28
 * @Description: 
*/

#ifndef _SQL_CONNECT_RAII_H_
#define _SQL_CONNECT_RAII_H_

#include "sql_connect_pool.h"

class SqlConnectRAII {
public:
    SqlConnectRAII(MYSQL **sql, SqlConnectPool *connect_pool);
    ~SqlConnectRAII();

private:
    MYSQL *sql_;
    SqlConnectPool *connect_pool_;
};

SqlConnectRAII::SqlConnectRAII(MYSQL **sql, SqlConnectPool *connect_pool) {
    assert(connect_pool);
    *sql = connect_pool->getConnect();
    sql_ = *sql;
    connect_pool_ = connect_pool;
}

SqlConnectRAII::~SqlConnectRAII() {
    if (sql_) {
        connect_pool_->freeConnect(sql_);
    }
}

#endif // _SQL_CONNECT_RAII_H_