#include "TimeHeap.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>

#define FD_LIMIT 65535
#define MAX_EVENT_NUMBER 1024
#define TIMESLOT 5

static int pipeFd[2];
static time_heap client_time_heap(1024);
static int epollFd = 0;
static int is_sig_alarm = 0;

int do_error(int fd, int* error) {
    fprintf(stderr, "error: %s\n", strerror(errno));
    *error = errno;
    while ((close(fd) == -1) && (errno == EINTR));
    errno = *error;
    return 1;
}

int setNonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addFd(int epollFd, int fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    // 向epoll对象中添加、修改或者删除感兴趣的时间
    epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
    setNonblocking(fd);
}

void sig_handler(int sig) {
    int save_errno = errno;
    int msg = sig;
    send(pipeFd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

void addSig(int sig) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof sa);
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    // 检查或者修改与指定信号相关联的处理动作
    assert(sigaction(sig, &sa, NULL) != -1);
}

void timer_handler() {
    printf("Entry the timer_handler() \n");
    client_time_heap.tick();
    heap_timer* tmp = NULL;
    if ((is_sig_alarm == 0) && (tmp = client_time_heap.top())) {
        time_t delay = tmp->expire - time(NULL);
        if (delay <= 0) {
            delay = 1;
        }
        alarm(delay);
        is_sig_alarm = 1;
    }
}

void cb_func(client_data* user_data) {
    epoll_ctl(epollFd, EPOLL_CTL_DEL, user_data->sock_fd, 0);
    assert(user_data);
    close(user_data->sock_fd);
    is_sig_alarm = 0;
    printf("cb_func: close fd %d\n", user_data->sock_fd);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof address);
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(static_cast<short>(port));

    int listenFd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenFd >= 0);

    int opt = 1;
    // SO_REUSEADDR: 允许服务器bind一个地址，即使这个地址当前已经存在已建立的连接
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

    ret = bind(listenFd, reinterpret_cast<sockaddr*>(&address), sizeof address);
    assert(ret != -1);

    ret = listen(listenFd, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    // 对使用方来说，epoll是一个黑盒子，我们通过操作系统提供的API，
    // 拿到一个实例（黑盒子）之后，就可以往里面注册我们想要监听的fd和事件，条件满足的时候，epoll就会通知我们
    epollFd = epoll_create(5);
    assert(epollFd != -1);

    addFd(epollFd, listenFd);

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipeFd);
    assert(ret != -1);
    setNonblocking(pipeFd[1]);
    addFd(epollFd, pipeFd[0]);

    addSig(SIGALRM);
    addSig(SIGTERM);
    addSig(SIGPIPE);
    bool stop_server = false;
    auto* users = new client_data[FD_LIMIT];
    bool timeout = false;
    alarm(10);  // 10s后会收到一个SIGALRM信号

    printf("Server start run.....\n");
    while (!stop_server) {
        int number = epoll_wait(epollFd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR)) {
            printf("epoll failure\n");
        }
        printf("\n");
        for (int i = 0; i < number; ++i) {
            int sockFd = events[i].data.fd;
            if (sockFd == listenFd) {
                printf("sockFd = new client\n");
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof client_addr;
                int connFd = accept(listenFd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
                if (connFd != -1) {
                    //  必须判断
                    addFd(epollFd, connFd);

                    users[connFd].address = client_addr;
                    users[connFd].sock_fd = connFd;

                    auto *timer = new heap_timer(TIMESLOT);
                    if (timer) {
                        timer->user_data = &users[connFd];
                        timer->cb_func = cb_func;
                        client_time_heap.add_timer(timer);
                        if (is_sig_alarm == 0) {
                            alarm(TIMESLOT);  // 5s后触发SIGALRM信号，进入
                            is_sig_alarm = 1;
                        }
                        users[connFd].timer = timer;
                        fprintf(stderr, "client: %d add tw_timer successfully\n", connFd);
                    } else {
                        fprintf(stderr, "client: %d add tw_timer failed\n", connFd);
                    }
                }
            } else if ((sockFd == pipeFd[0]) && (events[i].events & EPOLLIN)) {
                printf("sockFd = pipeFd[0] \n");
                int sig;
                char signals[1024];
                ret = recv(pipeFd[0], signals, sizeof signals, 0);
                if (ret == -1) {
                    continue;
                } else if (ret == 0) {
                    continue;
                } else {
                    for (int i = 0; i < ret; ++i) {
                        printf("signals[i] = %d \n", (int) (signals[i]));
                        switch (signals[i]) {
                            case SIGALRM:
                                printf("SIGALRM \n");
                                timeout = true;
                                break;
                            case SIGTERM:
                                stop_server = true;
                                break;
                            default:
                                break;
                        }
                    }
                }
            } else if (events[i].events & EPOLLIN) {
                printf("sockFd = client data\n");
                memset(users[sockFd].buf, '\0', BUFFER_SIZE);
                ret = recv(sockFd, users[sockFd].buf, BUFFER_SIZE - 1, 0);
                printf("get %d bytes of client data %s from %d \n", ret, users[sockFd].buf, sockFd);
                heap_timer* timer = users[sockFd].timer;
                if (ret < 0) {
                    if (errno != EAGAIN) {
                        cb_func((&users[sockFd]));
                        client_time_heap.del_timer(timer);
                        client_time_heap.pop_timer();
                    }
                } else if (ret == 0) {
                    cb_func(&users[sockFd]);
                    client_time_heap.del_timer(timer);
                    client_time_heap.pop_timer();
                } else {
                    if (timer) {
                        printf("connection.. to do adjust timer\n ");
                    }
                }
            } else {
                printf("something others happened\n");
            }
        }
        if (timeout) {
            timer_handler();
            timeout = false;
        }
    }
    close(listenFd);
    close(pipeFd[1]);
    close(pipeFd[0]);
    delete[] users;
    printf("End \n");
    return 0;
}