// 原子操作：不可中断的一个或者一系列操作
// 可重入函数：可以被中断的函数。
// ps aux | grep Signal可以找到当前运行函数所占用的端口
// sudo kill -l PID， 其中PID根据上面的命令查看，-l参数即信号变量，比如15表示SIGTERM，终止进程，即sudo kill -15 PID
// 如果还是不能终止，使用sudo kill -9 PID

#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<stdlib.h>
#include<sys/epoll.h>
#include<pthread.h>

#define MAX_EVENT_NUMBER 1024
static int pipeFd[2];

int setNonblocking(int fd) {
    int oldOption = fcntl(fd, F_GETFL);
    int newOption = oldOption | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOption);
    return oldOption;
}

void addFd(int epollFd, int fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    // 将新事件加入到事件表中
    epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
    setNonblocking(fd);
}

// 将信号值写入管道，通知主循环
void sigHandler(int sig) {
    int save_errno = errno;
    int msg = sig;
    send(pipeFd[1], reinterpret_cast<char*>(&msg), 1, 0);
    errno = save_errno;
}

void addSig(int sig) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof sa);
    sa.sa_handler = sigHandler; // 设置信号处理函数
    sa.sa_flags |= SA_RESTART; // 自动重启动
    sigfillset(&sa.sa_mask); // 初始化信号屏蔽集
    assert(sigaction(sig, &sa, NULL) != -1);
}


int main(int argc, char* argv[]) {
    if (argc <= 2) {
        printf("usage:%s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int ret = 0;
    sockaddr_in address;
    bzero(&address, sizeof address);
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(static_cast<short>(port));
    int listenFd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenFd >= 0);
    ret = bind(listenFd, reinterpret_cast<sockaddr*>(&address), sizeof address);
    assert(ret >= 0);
    ret = listen(listenFd, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    // 创建epoll事件表
    int epollFd = epoll_create(5);
    assert(epollFd != -1);
    addFd(epollFd, listenFd);
    /* sockpair 函数创建的管道是全双工的，不区分读写端
       此处我们假设pipe_fd[1]为写端，非阻塞
       pipe_fd[0]为读端
   */
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipeFd);
    assert(ret != -1);
    setNonblocking(pipeFd[1]);
    addFd(epollFd, pipeFd[0]);
    addSig(SIGHUP);  // 终端接口检测到一个连接断开，发送此信号
    addSig(SIGCHLD); // 子进程终止或停止时，子进程发送此信号
    addSig(SIGTERM); // 接收到kill命令
    addSig(SIGINT);  // 用户按下中断键(Delete或Ctrl+C)
    bool stopServer = false;
    while (!stopServer) {
        int number = epoll_wait(epollFd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR)) {
            printf("epoll failure\n");
            break;
        }
        for (int i = 0; i < number; ++i) {
            int sockFd = events[i].data.fd;
            if (sockFd == listenFd) {
                struct sockaddr_in client_addr;
                socklen_t len = sizeof client_addr;
                int connFd = accept(listenFd, reinterpret_cast<sockaddr*>(&client_addr), &len);
                if (connFd != -1) {
                    printf("new socket arrive:%d\n", connFd);
                }
                addFd(epollFd, connFd);
            } else if ((sockFd == pipeFd[0]) && (events[i].events & EPOLLIN)) {
                int sig;
                char signals[1024];
                ret = recv(pipeFd[0], signals, sizeof signals, 0);
                printf("recv message: %d\n", ret);
                if (ret == -1) {
                    continue;
                } else if (ret == 0) {
                    continue;
                } else {
                    // 每个信号值占1字节，所以按字节来逐个接收信号
                    for (int i = 0; i < ret; ++i) {
                        printf("server:I caugh the signal %d\n", signals[i]);
                        switch (signals[i]) {
                            case SIGCHLD:
                                // sudo kill -17 PID
                                printf("SIGCHLD signal\n");
                                break;
                            case SIGHUP:
                                // sudo kill -1 PID
                                printf("SIGHUP signal\n");
                                break;
                            case SIGTERM:
                                // sudo kill -15 PID
                                printf("SIGTERM signal\n");
                                stopServer = true;
                            case SIGINT:
                                printf("SIGINT signal\n");
                                stopServer = true;
                        }
                    }
                }
            } else {

            }
        }
    }
    printf("close fds\n");
    close(listenFd);
    close(pipeFd[1]);
    close(pipeFd[0]);
    return 0;
}

