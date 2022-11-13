#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>
#define MAX_EVENT_NUMBER 1024
#define TCP_BUFFER_SIZE 512
#define UDP_BUFFER_SIZE 1024

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
    epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
    setNonblocking(fd);
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
    assert(ret != -1);
    ret = listen(listenFd, 5);
    assert(ret != -1);
    bzero(&address, sizeof address);
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(static_cast<short >(port));
    int udpFd = socket(PF_INET, SOCK_DGRAM, 0);
    assert(udpFd >= 0);
    ret = bind(udpFd, reinterpret_cast<sockaddr*>(&address), sizeof address);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollFd = epoll_create(5);
    assert(epollFd != -1);
    addFd(epollFd, listenFd);
    addFd(epollFd, udpFd);
    while (1) {
        int number = epoll_wait(epollFd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0) {
            printf("epoll failure\n");
            break;
        }
        for (int i = 0; i < number; ++i) {
            int sockFd = events[i].data.fd;
            if (sockFd == listenFd) {
                printf("connect arrived\n");
                sockaddr_in client_addr{};
                socklen_t  client_len = sizeof client_addr;
                int connFd = accept(listenFd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
                addFd(epollFd, connFd);
            } else if (sockFd == udpFd) {
                printf("Udp message\n");
                char buf[UDP_BUFFER_SIZE];
                memset(buf, '\0', UDP_BUFFER_SIZE);
                sockaddr_in client_addr{};
                socklen_t client_len = sizeof client_addr;
                ret = recvfrom(udpFd, buf, UDP_BUFFER_SIZE - 1, 0,
                               reinterpret_cast<sockaddr*>(&client_addr), &client_len);
                if (ret > 0) {
                    sendto(udpFd, buf, UDP_BUFFER_SIZE - 1, 0,
                           reinterpret_cast<sockaddr*>(&client_addr), client_len);
                }
            } else if (events[i].events & EPOLLIN) {
                printf("Epoll_in\n");
                char buf[TCP_BUFFER_SIZE];
                while (1) {
                    memset(buf, '\0', sizeof buf);
                    ret = recv(sockFd, buf, TCP_BUFFER_SIZE - 1, 0);
                    if (ret < 0) {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                            break;
                        }
                        close(sockFd);
                        break;
                    } else if (ret == 0) {
                        close(sockFd);
                    } else {
                        send(sockFd, buf, ret, 0);
                    }
                }
            } else {
                printf("something else happened\n");
            }
        }
    }
    close(listenFd);
    return 0;
}

