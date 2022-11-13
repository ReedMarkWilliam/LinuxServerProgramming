#include "bits/stdc++.h"
#include <Ws2tcpip.h>
#include "io.h"

using namespace std;

// 建立服务端socket，192.168.2.16 12345
// 监听socket，等待client连接
// ChangeTcpSendBuffer中connect服务端socket并发送数据

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
        printf("usage:%s ip_address port_number\n", argv[0]);
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
    if (sock == -1) {
        printf("sock create failed, error:%d\n", WSAGetLastError());
        return 1;
    }
    int ret = bind(sock, (sockaddr*)&address, sizeof(address));
    if (ret == -1) {
        printf("bind failed, error:%d\n", WSAGetLastError());
        return 1;
    }
    ret = listen(sock, 5);
    if (ret == -1) {
        printf("ret failed, error:%d\n", WSAGetLastError());
        return 1;
    }
    sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (sockaddr*)&client, &client_addrlength);
    if (connfd < 0) {
        printf("connfd failed, error:%d\n", WSAGetLastError());
        return 1;
    } else {
        closesocket(STDOUT_FILENO);
        _dup(connfd);   // 在IO.h中
        printf("abcd\n");
        closesocket(connfd);
    }
    closesocket(sock);
    return 0;
}
