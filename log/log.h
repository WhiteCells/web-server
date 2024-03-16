/*
 * @Author: WhiteCells
 * @Date: 2024-03-09 13:01:08
 * @Last Modified by: WhiteCells
 * @Last Modified time: 2024-03-09 13:01:08
 * @Description: 
*/

#ifndef _LOG_H_
#define _LOG_H_

#include "block_deque.h"
#include "../buffer/buffer.h"
#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <cstring>
#include <cstdarg>
#include <cassert>
#include <sys/stat.h>

class Log {
public:
    static Log *instance();
    static void flushLogThread();
    
    void init(int level = 1, const char *path = "./log",
              const char *suffix = ".log",
              int max_deque_capacity = 1024);
    void write(int level, const char *format, ...);
    void flush();
    int getLevel();
    void setLevel(int level);
    bool isOpen();

private:
    Log();
    virtual ~Log();
    void appendLogLevelTitle_(int level);
    void asyncWrite_();
    
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int LOG_MAX_LINES = 50000;

    const char *path_;
    const char *suffix_;

    int max_lines_;
    int line_count_;
    int today_;
    int level_;
    bool is_open_;
    bool is_async_;

    Buffer buffer_;

    FILE *fp_;
    std::unique_ptr<BlockDeque<std::string>> deque_;
    std::unique_ptr<std::thread> write_thread_;
    std::mutex mtx_;
};

#define LOG_BASE(level, format, ...)\
    do {\
        Log *log = Log::instance();\
        if (log->isOpen() && log->getLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__);\
            log->flush();\
        }\
    } while (0);

#define LOG_DEBUG(format, ...) \
    do {\
        LOG_BASE(0, format, ##__VA_ARGS__)\
    } while (0);

#define LOG_INFO(format, ...)\
    do {\
        LOG_BASE(1, format, ##__VA_ARGS__)\
    } while (0);

#define LOG_WARN(format, ...)\
    do {\
        LOG_BASE(2, format, ##__VA_ARGS__)\
    } while(0);

#define LOG_ERROR(format, ...)\
    do {\
        LOG_BASE(3, format, ##__VA_ARGS__)\
    } while (0);

#endif // _LOG_H_