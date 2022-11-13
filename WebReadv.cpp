#include "bits/stdc++.h"
#include <Ws2tcpip.h>

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
    sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.2.60", &serveraddr.sin_addr);
    serveraddr.sin_port = 12346;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        printf("socket create failed, error:%d", WSAGetLastError());
        return 1;
    }
    int ret = connect(fd, (sockaddr*)&serveraddr, sizeof(serveraddr));
    if (ret == -1) {
        printf("connect failed, error:%d\n", WSAGetLastError());
        return 1;
    }
    char recvBuf[1024] = {0};
    while (recv(fd, recvBuf, sizeof(recvBuf), 0) > 0) {
        cout << "Has received" << '\n';
        cout << recvBuf << '\n';
    }
    closesocket(fd);
    return 0;
}
