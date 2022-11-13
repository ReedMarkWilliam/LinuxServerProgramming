#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<assert.h>
#include<stdio.h>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>

// 腾讯云中连接192.168.2.60 12345会超时

int timeout_connect(const char* ip, int port, int time) {
    int ret = 0;
    sockaddr_in address;
    bzero(&address, sizeof address);
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(static_cast<short>(port));
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    timeval timeout;
    timeout.tv_sec = time;
    timeout.tv_usec = 0;
    socklen_t len = sizeof timeout;
    ret = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
    assert(ret != -1);
    ret = connect(sock, reinterpret_cast<sockaddr*>(&address), sizeof address);
    if (ret == -1) {
        if (errno == EINPROGRESS) {
            printf("connecting timeout, process timeout logic\n");
            return -1;
        }
        printf("error occur when connecting to server\n");
        return -1;
    }
    return sock;
}

int main(int argc, char* argv[]) {
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int sock = timeout_connect(ip, port, 10);
    if (sock < 0) {
        return 1;
    }
    return 0;
}