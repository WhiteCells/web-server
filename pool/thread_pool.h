/*
 * @Author: WhiteCells
 * @Date: 2024-03-11 14:15:13
 * @Last Modified by: WhiteCells
 * @Last Modified time: 2024-03-11 14:15:13
 * @Description:
*/

#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <memory>
#include <cassert>

class ThreadPool {
public:
    explicit ThreadPool(size_t thread_count = 8)
        : pool_(std::make_shared<Pool>()) {
        assert(thread_count > 0);
        for (size_t i = 0; i < thread_count; ++i) {
            std::thread([pool = pool_] {
                std::unique_lock<std::mutex> locker(pool->mtx);
                while (true) {
                    if (!pool->tasks.empty()) {
                        auto task = std::move(pool->tasks.front());
                        pool->tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    } else if (pool->is_closed) {
                        break;
                    } else {
                        pool->condition.wait(locker);
                    }
                }
            }).detach();
        }
    }

    ThreadPool() = default;

    ThreadPool(ThreadPool &&) = default;

    ~ThreadPool() {
        if (static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->is_closed = true;
            }
            pool_->condition.notify_all();
        }
    }

    template<class T>
    void addTask(T &&task) {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<T>(task));
        }
        pool_->condition.notify_one();
    }

private:
    struct Pool {
        std::mutex mtx;
        std::condition_variable condition;
        std::queue<std::function<void()>> tasks;
        bool is_closed;
    };

    std::shared_ptr<Pool> pool_;
};

// ThreadPool::ThreadPool(size_t thread_count) :
//     pool_(std::make_shared<Pool>()) {
//     assert(thread_count > 0);
//     for (size_t i = 0; i < thread_count; ++i) {
//         std::thread([pool = pool_] {
//             std::unique_lock<std::mutex> locker(pool->mtx);
//             while (true) {
//                 if (!pool->tasks.empty()) {
//                     auto task = std::move(pool->tasks.front());
//                     pool->tasks.pop();
//                     locker.unlock();
//                     task();
//                     locker.lock();
//                 } else if (pool->is_closed) {
//                     break;
//                 } else {
//                     pool->condition.wait(locker);
//                 }
//             }
//         }).detach();
//     }
// }

// ThreadPool::~ThreadPool() {
//     if (static_cast<bool>(pool_)) {
//         {
//             std::lock_guard<std::mutex> locker(pool_->mtx);
//             pool_->is_closed = true;
//         }
//         pool_->condition.notify_all();
//     }
// }

// template<class T>
// void ThreadPool::addTask(T &&task) {
//     {
//         std::lock_guard<std::mutex> locker(pool_->mtx);
//         pool_->tasks.emplace(std::forward<T>(task));
//     }
//     pool_->condition.notify_one();
// }

#endif // _THREAD_POOL_H_