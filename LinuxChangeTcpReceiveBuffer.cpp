#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#define BUFFER_SIZE 512

using namespace std;

int main(int argc, char* argv[]) {
    if (argc <= 2) {
        printf("usgae:%s ip_address port_number send_buffer_size\n", argv[0]);
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = port;
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    int recBuf = atoi(argv[3]);
    int len = sizeof(recBuf);
    // SOL_SOCKET 通用socket选型
    // SO_SNDBUF TCP发送缓冲区，最小值是2048字节
    // SO_RCVBUF TCP接收缓冲区，最小值是256字节
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&recBuf, len);
    getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&recBuf, (socklen_t*)&len);
    printf("the tcp receive buffer size after setting is%d\n", recBuf);
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&recBuf, len);
    int ret = bind(sock, (sockaddr*)&address, sizeof(address));
    // 返回10049是因为IP地址错误，不是局域网内的ip都能随便bind的，bind必须绑定本机ip地址，127.0.0.1即可。
    assert(ret != -1);
    ret = listen(sock, 5);
    assert(ret != -1);
    sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (sockaddr*)&client, &client_addrlength);
    if (connfd < 0) {
        printf("errno is%d\n", errno);
    } else {
        char buffer[BUFFER_SIZE];
        memset(buffer, '\0', BUFFER_SIZE);
        while (recv(connfd, buffer, BUFFER_SIZE - 1, 0) > 0) {
            printf("Has received\n");
            printf("%s\n", buffer);
        }
        close(connfd);
    }
    close(sock);
    return 0;
}
