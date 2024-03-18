/*
 * @Author: WhiteCells
 * @Date: 2024-03-10 09:50:37
 * @Last Modified by: WhiteCells
 * @Last Modified time: 2024-03-10 09:50:37
 * @Description:
*/

#include "log.h"

Log *Log::instance() {
    static Log inst;
    return &inst;
}

void Log::flushLogThread() {
    Log::instance()->asyncWrite_();
}

void Log::init(int level, const char *path,
               const char *suffix,
               int max_deque_capacity) {
    level_ = level;
    is_open_ = true;;
    
    if (max_deque_capacity > 0) {
        is_async_ = true;
        if (!deque_) {
            std::unique_ptr<BlockDeque<std::string>> new_deque(new BlockDeque<std::string>);
            deque_ = std::move(new_deque);
            std::unique_ptr<std::thread> new_thread(new std::thread(flushLogThread));
            write_thread_ = move(new_thread);
        }
    } else {
        is_async_ = false;
    }

    line_count_ = 0;
    path_ = path;
    suffix_ = suffix;

    time_t timer = time(nullptr);
    struct tm *sys_time = localtime(&timer);
    struct tm t = *sys_time;

    char file_name[LOG_NAME_LEN] {0};
    snprintf(file_name, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
             path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    today_ = t.tm_mday;

    {
        std::lock_guard<std::mutex> locker(mtx_);
        buffer_.retrieveAll();
        if (fp_) {
            flush();
            fclose(fp_);
        }

        fp_ = fopen(file_name, "a");
        if (!fp_) {
            mkdir(path_, 0777);
            fp_ = fopen(file_name, "a");
        }
        assert(fp_);
    }
}

void Log::write(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t time_sec = now.tv_sec;
    struct tm *sys_time = localtime(&time_sec);
    struct tm t = *sys_time;
    va_list valist;

    if (today_ != t.tm_mday
        || (line_count_ % LOG_MAX_LINES == 0)) {
        std::unique_lock<std::mutex> locker(mtx_);
        locker.unlock();

        char new_file[LOG_NAME_LEN] {0};
        char tail[36] {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900,
                 t.tm_mon + 1, t.tm_mday);

        if (today_ != t.tm_mday) {
            snprintf(new_file, LOG_NAME_LEN - 72, "%s/%s%s",
                     path_, tail, suffix_);
            today_ = t.tm_mday;
            line_count_ = 0;
        } else {
            snprintf(new_file, LOG_NAME_LEN - 72, "%s/%s-%d%s",
                     path_, tail, (line_count_ / LOG_MAX_LINES), suffix_);
        }

        locker.lock();
        flush();
        fclose(fp_);
        fp_ = fopen(new_file, "a");
        assert(fp_);
    }

    {
        std::unique_lock<std::mutex> lcoker(mtx_);
        ++line_count_;
        int n = snprintf(buffer_.beginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld",
                         t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                         t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        buffer_.hasWritten(n);
        appendLogLevelTitle_(level);
        va_start(valist, format);
        int m = vsnprintf(buffer_.beginWrite(), buffer_.getWriteableBytes(),
                          format, valist);
        va_end(valist);
        buffer_.hasWritten(m);
        buffer_.append("\n\0", 2);

        if (is_async_ && deque_ && !deque_->full()) {
            deque_->push_back(buffer_.retrieveAllToStr());
        } else {
            fputs(buffer_.peek(), fp_);
        }
        buffer_.retrieveAll();
    }
}

void Log::flush() {
    if (is_async_) {
        deque_->flush();
    }
    fflush(fp_);
}

int Log::getLevel() {
    std::lock_guard<std::mutex> locker(mtx_);
    return level_;
}

void Log::setLevel(int level) {
    std::lock_guard<std::mutex> locker(mtx_);
    level_ = level;
}

bool Log::isOpen() {
    return is_open_;
}

Log::Log() {
    line_count_ = 0;
    is_async_ = false;
    write_thread_ = nullptr;
    deque_ = nullptr;
    today_ = 0;
    fp_ = nullptr;
}

Log::~Log() {
    if (write_thread_ && write_thread_->joinable()) {
        while (!deque_->empty()) {
            deque_->flush();
        }
        deque_->close();
        write_thread_->join();
    }

    if (fp_) {
        std::lock_guard<std::mutex> locker(mtx_);
        flush();
        fclose(fp_);
    }
}

void Log::appendLogLevelTitle_(int level) {
    switch (level) {
        case 0:
            buffer_.append("[debug]: ", 9);
            break;
        default:
        case 1:
            buffer_.append("[info]: ", 9);
            break;
        case 2:
            buffer_.append("[warn]: ", 9);
            break;
        case 3:
            buffer_.append("[error]: ", 9);
            break;
    }
}

void Log::asyncWrite_() {
    std::string str {};
    while (deque_->pop(str)) {
        std::lock_guard<std::mutex> locker(mtx_);
        fputs(str.c_str(), fp_);
    }
}