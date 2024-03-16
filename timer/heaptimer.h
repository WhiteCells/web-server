/*
 * @Author: WhiteCells
 * @Date: 2024-03-10 16:01:30
 * @Last Modified by: WhiteCells
 * @Last Modified time: 2024-03-10 16:01:30
 * @Description: 
*/

#ifndef _HEAPTIMER_H_
#define _HEAPTIMER_H_

#include "../log/log.h"
#include <queue>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <chrono>
#include <cassert>
#include <ctime>
#include <arpa/inet.h>

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef Clock::time_point TimeStamp;
typedef std::chrono::milliseconds MS;

struct TimerNode {
    int id;
    TimeStamp expires;
    TimeoutCallBack cb;
    bool operator<(const TimerNode &t) {
        return expires < t.expires;
    }
};

class HeapTimer {
public:
    HeapTimer();

    ~HeapTimer();

    void adjust(int id, int timeout);

    void add(int id, int timeout, const TimeoutCallBack &cb);

    void doWork(int id);

    void clear();

    void tick();

    void pop();

    int getNextTick();

private:
    void del_(size_t index);

    void siftup_(size_t i);

    bool siftdown_(size_t index, size_t n);

    void swapTimerNode_(size_t i, size_t j);

    std::vector<TimerNode> heap_;

    std::unordered_map<int, size_t> ref_;
};

#endif // _HEAPTIMER_H_