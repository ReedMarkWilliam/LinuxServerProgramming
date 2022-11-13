#include <sys/types.h>
#include <event2/event-config.h>
#include <stdio.h>
#include <event.h>
#include <sys/signal.h>
#include "src/evthread-internal.h"
#include "src/event-internal.h"

struct event ev;
struct timeval tv;

#define TIMER_ON FALSE

#if TIMER_ON
void time_cb(int fd, short event, void *argc)
{
    printf("timer wakeup!\n");
    event_add(&ev, &tv);
}
int main()
{
    struct event_base *base = event_init();
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    evtimer_set(&ev, time_cb, NULL);
    event_base_set(base, &ev);
    event_add(&ev, &tv);
    event_base_dispatch(base);
}
#endif

void signal_cb(int fd, short event, void* argc) {
    struct event_base* base = reinterpret_cast<event_base*>(argc);
    struct timeval delay = {2, 0};
    printf("Caught an interrupt signal; exiting cleanly in two second...\n");
    event_base_loopexit(base, &delay);
}

void timeout_cb(int fd, short event, void* argc) {
    printf("timeout\n");
    //event_add(&ev, &tv);
}


/* 需要引入相应的头文件 */
#define EVLOCK_ASSERT_LOCKED(lock)					\
	do {								\
		if ((lock) && evthread_lock_debugging_enabled_) {	\
			EVUTIL_ASSERT(evthread_is_debug_lock_held_(lock)); \
		}							\
	} while (0)


#define EVENT_BASE_ASSERT_LOCKED(base)		\
	EVLOCK_ASSERT_LOCKED((base)->th_base_lock)

int
event_add_internal(struct event *ev, const struct timeval *tv,
                  int tv_is_absolute) {
    struct event_base *base = ev->ev_base;
    int res = 0;
    int notify = 0;
    EVENT_BASE_ASSERT_LOCKED(base);
    /* ev_flags是一个宏定义，在event-internal.h中定义 */
    EVUTIL_ASSERT(!(ev->ev_flags & ~EVLIST_ALL));
    if (ev->ev_flags & EVLIST_FINALIZING) {
        return -1;
    }
    if (tv != NULL && !(ev->ev_flags & EVLIST_TIMEOUT)) {
        if (min_heap_reserve_(&base->timeheap, 1 + min_heap_size_(&base->timeheap)) == -1) {
            return -1;
        }
    }

}

int main() {
    struct event_base* base = event_init();  // 创建event_base对象，一个该对象相当于一个Reactor实例
    // Reactor模式即应答者模式，是基于事件驱动的设计模式
    struct event* signal_event = evsignal_new(base, SIGINT, signal_cb, base); // 创建具体的事件处理器
    // base参数指定从属的Reactor， fd参数指定与该事件处理器关联的句柄，IO事件处理器给文件描述符，信号事件处理器给信号值，定时信号处理器给-1
    event_add(signal_event, NULL);
    timeval tv = {1, 0};
//    struct event* timeout_event = evtimer_new(base, timeout_cb, NULL);
//    event_add(timeout_event, &tv);
    event_set(&ev, -1, 0, timeout_cb, NULL);  //初始化event结构中成员
    event_base_set(base, &ev);
    event_add(&ev, &tv);  //将event添加到events事件链表，注册事件
    event_base_dispatch(base);   // 执行事件循环
    event_free(signal_event);
    event_base_free(base);
}