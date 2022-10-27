#include "bits/stdc++.h"
#include <Ws2tcpip.h>
#define BUFFER_SIZE 512

int main(int argc, char* argv[]) {
    // 首先要加载套接字库，设置套接字版本信息等
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    wVersionRequested = MAKEWORD(2, 1); //高位字节存储副版本号, 低位字节存储主版本号
    err = WSAStartup(wVersionRequested, &wsaData);//WSAStartup，即WSA(Windows Sockets Asynchronous，Windows异步套接字)的启动命令
    if (err != 0) return 1;
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 1) { //高位字节和低位字节不正确
        WSACleanup();
        return 1;
    }
    if (argc <= 2) {
        printf("usgae:%s ip_address port_number send_buffer_size\n", argv[0]);
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_address.sin_addr);
    server_address.sin_port = htonl(port);
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    int sendBuf = atoi(argv[3]);
    int len = sizeof(sendBuf);
    // SOL_SOCKET 通用socket选型
    // SO_SNDBUF TCP发送缓冲区，最小值是2048字节
    // SO_RCVBUF TCP接收缓冲区，最小值是256字节
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&sendBuf, len);
    getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&sendBuf, &len);
    printf("the tcp send buffer size after setting is%d\n", sendBuf);
    if (connect(sock, (sockaddr*)&server_address, sizeof(server_address)) != -1) {
        char buffer[BUFFER_SIZE];
        memset(buffer, 'a', BUFFER_SIZE);
        send(sock, buffer, BUFFER_SIZE, 0);
    }
    closesocket(sock);
    return 0;
}