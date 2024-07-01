#ifndef TIMER_H
#define TIMER_H
#include <functional>
#include <chrono>
#include <set>
#include <memory>
#include <iostream>

extern "C"
{
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <time.h> // for timespec itimerspec
#include <unistd.h> // for close
}

using namespace std;


struct TimerNodeBase {
    time_t expire;//过期时间
    uint64_t id; //一个定时事件的ID，用于在过期时间相同的情况下进行区分
};

struct TimerNode : public TimerNodeBase {
    using Callback = std::function<void(const TimerNode &node)>;
    Callback _func;//定时器触发的回调函数
    TimerNode(int64_t id, time_t expire, Callback func) : _func(func) {
        this->expire = expire;
        this->id = id;
    }
};

//利用set维护有序性时需要一个函数进行比较
bool operator < (const TimerNodeBase &lhd, const TimerNodeBase &rhd) {
    if (lhd.expire < rhd.expire) {
        return true;
    } else if (lhd.expire > rhd.expire) {
        return false;
    } else return lhd.id < rhd.id;
}

class Timer {
public:
    static inline time_t GetTick() {    //返回当前时间，使用steady_clock
        return chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count();
    }
    //添加一个Timer事件
    TimerNodeBase AddTimer(int msec, TimerNode::Callback func) {
        time_t expire = GetTick() + msec;
        if (timeouts.empty() || expire <= timeouts.crbegin()->expire) {
            auto pairs = timeouts.emplace(GenID(), expire, std::move(func));
            return static_cast<TimerNodeBase>(*pairs.first);
        }
        auto ele = timeouts.emplace_hint(timeouts.crbegin().base(), GenID(), expire, std::move(func));
        return static_cast<TimerNodeBase>(*ele);
    }
    //删除一个Timer事件
    void DelTimer(TimerNodeBase &node) {
        auto iter = timeouts.find(node);
        if (iter != timeouts.end())
            timeouts.erase(iter);
    }
    //处理一个Timer事件
    void HandleTimer(time_t now) {
        auto iter = timeouts.begin();
        while (iter != timeouts.end() && iter->expire <= now) {
            iter->_func(*iter);
            iter = timeouts.erase(iter);
        }
    }

public:
    virtual bool  UpdateTimerfd(const int fd) {
        struct timespec abstime;
        auto iter = timeouts.begin();
        if (iter != timeouts.end()) {
            abstime.tv_sec = iter->expire / 1000;
            abstime.tv_nsec = (iter->expire % 1000) * 1000000;
        } else {
            abstime.tv_sec = 0;
            abstime.tv_nsec = 0;
            return false;
        }
        struct itimerspec its = {
            .it_interval = {},
            .it_value = abstime
        };
        timerfd_settime(fd, TFD_TIMER_ABSTIME, &its, nullptr);
        return true;
    }

private:
    static inline uint64_t GenID() {
        return gid++;
    }
    static uint64_t gid; 

    set<TimerNode, std::less<>> timeouts;
};
uint64_t Timer::gid = 0;

#endif