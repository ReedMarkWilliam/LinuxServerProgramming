#include "bits/stdc++.h"
#include <Ws2tcpip.h>
#define BUFFER_SIZE 512

using namespace std;

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
    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htonl(port);
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    int recBuf = atoi(argv[3]);
    int len = sizeof(recBuf);
    // SOL_SOCKET 通用socket选型
    // SO_SNDBUF TCP发送缓冲区，最小值是2048字节
    // SO_RCVBUF TCP接收缓冲区，最小值是256字节
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&recBuf, len);
    getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&recBuf, &len);
    printf("the tcp receive buffer size after setting is%d\n", recBuf);
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&recBuf, len);
    int ret = bind(sock, (sockaddr*)&address, sizeof(address));
    if (ret == -1) cout << WSAGetLastError() << endl; // Linux下是error code， Windows下是WSAGetLastError()
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
        while (recv(connfd, buffer, BUFFER_SIZE - 1, 0) > 0) {}
        closesocket(connfd);
    }
    closesocket(sock);
    return 0;
}