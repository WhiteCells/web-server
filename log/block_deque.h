/*
 * @Author: WhiteCells
 * @Date: 2024-03-09 12:36:52
 * @Last Modified by: WhiteCells
 * @Last Modified time: 2024-03-09 12:36:52
 * @Description:
*/

#ifndef _BLOCK_DEQUE_H_
#define _BLOCK_DEQUE_H_

#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>
#include <assert.h>

template<class T>
class BlockDeque {
public:
    explicit BlockDeque(size_t max_capacity = 1000);
    ~BlockDeque();
    void close();
    void flush();
    void clear();
    bool full();
    bool empty();
    size_t size();
    size_t capacity();
    T front();
    T back();
    void push_back(const T &item);
    void push_front(const T &item);
    bool pop(T &item);
    bool pop(T &item, int timeout);

private:
    std::deque<T> deque_;
    size_t capacity_;
    std::mutex mtx_;
    bool is_close_;
    std::condition_variable consumer_;
    std::condition_variable producer_;
};

template<class T>
BlockDeque<T>::BlockDeque(size_t max_capacity) : capacity_(max_capacity) {
    assert(max_capacity > 0);
    is_close_ = false;
}

template<class T>
BlockDeque<T>::~BlockDeque() {
    close();
}

template<class T>
void BlockDeque<T>::close() {
    {
        std::lock_guard<std::mutex> locker(mtx_);
        deque_.clear();
        is_close_ = true;
    }
    producer_.notify_all();
    consumer_.notify_all();
}

template<class T>
void BlockDeque<T>::flush() {
    consumer_.notify_all();
}

template<class T>
void BlockDeque<T>::clear() {
    std::lock_guard<std::mutex> locker(mtx_);
    deque_.clear();
}

template<class T>
bool BlockDeque<T>::full() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deque_.size() >= capacity_;
}

template<class T>
bool BlockDeque<T>::empty() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deque_.empty();
}

template<class T>
size_t BlockDeque<T>::size() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deque_.size();
}

template<class T>
size_t BlockDeque<T>::capacity() {
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}

template<class T>
T BlockDeque<T>::front() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deque_.front();
}

template<class T>
T BlockDeque<T>::back() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deque_.back();
}

template<class T>
void BlockDeque<T>::push_back(const T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deque_.size() >= capacity_) {
        producer_.wait(locker);
    }
    deque_.push_back(item);
    consumer_.notify_one();
}

template<class T>
void BlockDeque<T>::push_front(const T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deque_.size() >= capacity_) {
        producer_.wait(locker);
    }
    deque_.push_front(item);
    consumer_.notify_one();
}

template<class T>
bool BlockDeque<T>::pop(T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deque_.empty()) {
        consumer_.wait(locker);
        if (is_close_) {
            return false;
        }
    }
    item = deque_.front();
    deque_.pop_front();
    producer_.notify_one();
    return true;
}

template<class T>
bool BlockDeque<T>::pop(T &item, int timeout) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deque_.empty()) {
        if (consumer_.wait_for(locker, std::chrono::seconds(timeout))
            == std::cv_status::timeout) {
            return false;
        }
        if (is_close_) {
            return false;
        }
    }
    item = deque_.front();
    deque_.pop_front();
    producer_.notify_one();
    return true;
}

#endif // _BLOCK_DEQUE_H_