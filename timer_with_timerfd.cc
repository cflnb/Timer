#include "timer.h"

int main()
{
    int epfd = epoll_create(1);

    int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    struct epoll_event ev = {.events = EPOLLIN | EPOLLET};
    epoll_ctl(epfd, EPOLL_CTL_ADD, timerfd, &ev);

    unique_ptr<Timer> timer = make_unique<Timer>();
    int i = 0;
    timer->AddTimer(1000, [&](const TimerNode &node)
                    { cout << Timer::GetTick() << " node id:" << node.id << " revoked times:" << ++i << endl; });

    timer->AddTimer(1000, [&](const TimerNode &node)
                    { cout << Timer::GetTick() << " node id:" << node.id << " revoked times:" << ++i << endl; });

    timer->AddTimer(3000, [&](const TimerNode &node)
                    { cout << Timer::GetTick() << " node id:" << node.id << " revoked times:" << ++i << endl; });

    auto node = timer->AddTimer(2100, [&](const TimerNode &node)
                                { cout << Timer::GetTick() << " node id:" << node.id << " revoked times:" << ++i << endl; });
    timer->DelTimer(node);

    cout << "now time:" << Timer::GetTick() << endl;

    struct epoll_event evs[64] = {0};
    while (true)
    {
        // 每次循环从set当中取出截止时间最早的时间将其更新到fd当中
        if (!timer->UpdateTimerfd(timerfd))
            break;
        int n = epoll_wait(epfd, evs, 64, -1);
        time_t now = Timer::GetTick();
        for (int i = 0; i < n; i++)
        {
            // for network event handle
        }
        timer->HandleTimer(now);
    }
    epoll_ctl(epfd, EPOLL_CTL_DEL, timerfd, &ev);
    close(timerfd);
    close(epfd);
    return 0;
}
