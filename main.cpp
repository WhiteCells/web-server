/*
 * @Author: Author
 * @Date: 2024-03-18 10:28:01
 * @Last Modified by: Author
 * @Last Modified time: 2024-03-18 10:28:01
 * @Description: 
*/

#include <unistd.h>
#include "server/web_server.h"

int main(int argc, char *argv[]) {
    WebServer server(
        4000, 3, 60000, false,             /* 端口 ET timeout_ms_ 优雅退出 */
        3306, "root", "3215", "webserver", /* MySql端口 用户名 密码 库名 */
        12, 6, true, 1, 1024               /* 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量 */
    );
    server.start();
    return 0;
}