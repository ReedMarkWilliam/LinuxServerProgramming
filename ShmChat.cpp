#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<stdlib.h>
#include<sys/epoll.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<fcntl.h>
#define USER_LIMIT 5
#define BUFFER_SIZE 1024
#define FD_LIMIT 65535
#define MAX_EVENT_NUMBER 1024
#define PROCESS_LIMIT 65536

/* 使用了POSIX共享内存函数，则编译时需要指定选项-lrt，即target_link_libraries(ShmChat -levent -lrt) */

/* 客户连接的数据 */
struct client_data {
    sockaddr_in address;  /* 客户端socket地址*/
    int connfd;     /*客户端socket文件描述符 */
    pid_t pid;      /*处理这个连接的子进程的PID */
    int pipefd[2];  /* 和父进程通信用的管道 */
};

static const char* shm_name = "/my_shm";
int sig_pipefd[2];
int epollfd;
int listenfd;
int shmfd;
char* share_mem = 0;
/* 客户连接数组，进程使用客户连接的编号来索引这个数组，即可取得相关的客户连接数据  */
client_data* users = 0;
/* 子进程和客户连接的映射关系表，用进程的pid来索引这个数组，即可取得该进程所处理的客户连接的编号 */
int* sub_process = 0;
int user_count = 0;
bool stop_child = false;

int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL); /* 对已打开的文件描述符进行各种控制操作以改变已打开文件的各种属性*/
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);  /* 向epoll对象中添加、修改或者删除感兴趣的事件 */
    setnonblocking(fd);
}

void sig_handler(int sig) {
    int save_errno = errno;
    int msg = sig;
    send(sig_pipefd[1], (char *)&msg, 1, 0); /* 最后一个参数为flag，一般为0 */
    errno = save_errno;
}

void addsig(int sig, void(*handler)(int), bool restart = true) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof sa);
    /* sa_handler是旧的信号处理函数，原型为void(*handler)(int)
     * sa_sigaction是新的信号处理函数，原型为void(*sa_sigaction)(int,siginfo_t *,void *) */
    sa.sa_handler = handler;
    if (restart) {
        /* sa_flags：用于指定信号处理的行为，SA_RESART：使被信号打断的系统调用自动重新发起*/
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);       /* 信号阻塞集，在信号处理函数执行过程中，临时屏蔽指定信号 */
    assert(sigaction(sig, &sa, NULL) != -1); /* 为信号指定相关的处理程序 */
}

void del_resource() {
    close(sig_pipefd[0]);
    close(sig_pipefd[1]);
    close(listenfd);
    close(epollfd);
    shm_unlink(shm_name);
    delete [] users;
    delete [] sub_process;
}

void child_term_handler(int sig) {
    /* 停止一个子进程 */
    stop_child = true;
}

/* 子进程运行函数，idx指出该子进程处理的客户连接的编号，users是保存所有客户连接数据的数组，share_mem指出共享内存的起始地址 */
int run_child(int idx, client_data* users, char* share_mem) {
    epoll_event events[MAX_EVENT_NUMBER];
    /* 子进程使用IO复用技术来同时监听两个文件描述符：客户连接socket、与父进程通信的管道文件描述符 */
    int child_epollfd = epoll_create(5);
    assert(child_epollfd != -1);
    int connfd = users[idx].connfd;
    addfd(child_epollfd, connfd);
    int pipefd = users[idx].pipefd[1];
    addfd(child_epollfd, pipefd);
    int ret;
    /* 子进程需要设置自己的信号处理函数 */
    addsig(SIGTERM, child_term_handler, false);
    while (!stop_child) {
        int number = epoll_wait(child_epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR)) {
            printf("epoll failed\n");
            break;
        }
        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;
            /* 本子进程负责的客户连接有数据到达 */
            if ((sockfd == connfd) && (events[i].events & EPOLLIN)) {
                memset(share_mem + idx * BUFFER_SIZE, '\0', BUFFER_SIZE);
                /* 将客户数据读取到对应的读缓存中，该读缓存是共享内存的一段，开始于idx * BUFFER_SIZE处， 长度
                 * 为BUFFER_SIZE字节 */
                ret = recv(connfd, share_mem + idx * BUFFER_SIZE, BUFFER_SIZE - 1, 0);
                if (ret < 0) {
                    if (errno != EAGAIN) {
                        stop_child = true;
                    }
                } else if (ret == 0) {
                    stop_child = true;
                } else {
                    /* 成功读取客户数据后通知主线程通过管道来处理 */
                    send(pipefd, (char*)&idx, sizeof idx, 0);
                }
            } else if ((sockfd == pipefd) && (events[i].events & EPOLLIN)) {
                /* 主进程通过管道通知本进程将第client个客户的数据发送到本进程负责的客户端*/
                int client = 0;
                /* 接收主进程发送来的数据，即有客户数据到达的连接的编号*/
                ret = recv(sockfd, (char*)& client, sizeof client, 0);
                if (ret < 0) {
                    if (errno != EAGAIN) {
                        stop_child = true;
                    }
                } else if (ret == 0) {
                    stop_child = true;
                } else {
                    send(connfd, share_mem + client * BUFFER_SIZE, BUFFER_SIZE, 0);
                }
            } else {
                continue;
            }
        }
    }
    close(connfd);
    close(pipefd);
    close(child_epollfd);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc <= 2) {
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
    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    ret = bind(listenfd, reinterpret_cast<sockaddr*>(&address), sizeof address);
    assert(ret != -1);
    ret = listen(listenfd, 5);
    assert(ret != -1);
    user_count = 0;
    users = new client_data[USER_LIMIT];
    sub_process = new int[PROCESS_LIMIT];
    for (int i = 0; i < PROCESS_LIMIT; ++i) {
        sub_process[i] = -1;
    }
    epoll_event events[MAX_EVENT_NUMBER];
    epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd);
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
    assert(ret != -1);
    setnonblocking(sig_pipefd[1]);
    addfd(epollfd, sig_pipefd[0]);
    addsig(SIGCHLD, sig_handler);
    addsig(SIGTERM, sig_handler);
    addsig(SIGINT, sig_handler);
    addsig(SIGPIPE, SIG_IGN);
    bool stop_server = false;
    bool terminate = false;
    /* 创建共享内存，作为所有客户socket连接的读缓存 */
    shmfd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    assert(shmfd != -1);
    /* 将shmfd的大小改为USER_LIMIT * BUFFER_SIZE */
    ret = ftruncate(shmfd, USER_LIMIT * BUFFER_SIZE);
    assert(ret != -1);
    /* 地址映射*/
    share_mem = (char*) mmap(NULL, USER_LIMIT * BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    assert(share_mem != MAP_FAILED);
    close(shmfd);
    while (!stop_server) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        /*在socket服务器端，设置了信号捕获机制，有子进程，
         * 当在父进程阻塞于慢系统调用时由父进程捕获到了一个有效信号时，
         * 内核会致使accept返回一个EINTR错误(被中断的系统调用)。*/
        if ((number < 0) && (errno != EINTR)) {
            printf("epoll failure\n");
            break;
        }
        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;
            /* 新的客户连接到来 */
            if (sockfd == listenfd) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof client_addr;
                int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_len);
                if (connfd < 0) {
                    printf("errno is %d\n", errno);
                    continue;
                }
                if (user_count >= USER_LIMIT) {
                    const char* info = "too many users\n";
                    printf("%s", info);
                    send(connfd, info, strlen(info), 0);
                    close(connfd);
                    continue;
                }
                printf("new client arrive, socket:%d\n", connfd);
                users[user_count].address = client_addr;
                users[user_count].connfd = connfd;
                /* 在主进程和子进程之间建立管道，以传递必要的数据 */
                ret = socketpair(PF_UNIX, SOCK_STREAM, 0, users[user_count].pipefd);
                assert(ret != -1);
                pid_t pid = fork();     /* 创建子进程 */
                if (pid < 0) {
                    close(connfd);
                    continue;
                } else if (pid == 0) {
                    /* 说明父进程被关闭，由系统进程接管 */
                    close(epollfd);
                    close(listenfd);
                    close(users[user_count].pipefd[0]);
                    close(sig_pipefd[0]);
                    close(sig_pipefd[1]);
                    run_child(user_count, users, share_mem);
                    munmap((void*)share_mem, USER_LIMIT * BUFFER_SIZE);
                    exit(0);
                } else {
                    close(connfd);
                    close(users[user_count].pipefd[1]);
                    addfd(epollfd, users[user_count].pipefd[0]);
                    users[user_count].pid = pid;
                    sub_process[pid] = user_count;  /* pid与新的客户连接索引值之间的映射关系 */
                    user_count++;
                }
            } else if ((sockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN)) {
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0], signals, sizeof signals, 0);
                if (ret == -1) {
                    continue;
                } else if (ret == 0) {
                    continue;
                } else {
                    for (int i = 0; i < ret; ++i) {
                        switch (signals[i]) {
                            case SIGCHLD:
                                pid_t pid;
                                int stat;
                                /* 清理僵尸进程 */
                                while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
                                    int del_user = sub_process[pid];
                                    sub_process[pid] = -1;
                                    if ((del_user < 0) || (del_user > USER_LIMIT)) {
                                        continue;
                                    }
                                    epoll_ctl(epollfd, EPOLL_CTL_DEL, users[del_user].pipefd[0], 0);
                                    close(users[del_user].pipefd[0]);
                                    users[del_user] = users[--user_count];
                                    sub_process[users[del_user].pid] = del_user;
                                }
                                if (terminate && user_count == 0) {
                                    stop_server = true;
                                }
                                break;
                            case SIGTERM:
                            case SIGINT:
                                printf("kill all the child now\n");
                                if (user_count == 0) {
                                    stop_server = true;
                                    break;
                                }
                                for (int i = 0; i < user_count; ++i) {
                                    int pid = users[i].pid;
                                    kill(pid, SIGTERM);
                                }
                                terminate = true;
                                break;
                            default:
                                break;
;                        }
                    }
                }
            } else if(events[i].events && EPOLLIN) {
                int child = 0;
                ret = recv(sockfd, (char*)&child, sizeof child, 0);
                printf("read data from child accross pipe: %d\n", child);
                if (ret == -1) {
                    continue;
                } else if (ret == 0) {
                    continue;
                } else {
                    for (int j = 0; j < user_count; ++j) {
                        if (users[j].pipefd[0] != sockfd) {
                            printf("send data to child accross pipe\n");
                            send(users[j].pipefd[0], (char*)&child, sizeof child, 0);
                        }
                    }
                }
            }
        }
    }
    del_resource();
    return 0;
}


