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
    assert(argc == 2);
    char* host = argv[1];
    hostent* hostInfo = gethostbyname(host);
    assert(hostInfo);
    servent* servInfo = getservbyname("daytime", "tcp");
    assert(servInfo);
    printf("daytime port is%d\n", ntohs(servInfo->s_port));
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = servInfo->s_port;
    // h_addr_list本身使用的就是网络字节序
    address.sin_addr = *(in_addr*)*hostInfo->h_addr_list;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int result = connect(sockfd, (sockaddr*)&address, sizeof(address));
    assert(result != -1);
    char buffer[128];
    result = recv(sockfd, buffer, sizeof(buffer), 0);
    assert(result > 0);
    buffer[result] = '\0';
    printf("the day time is:%s", buffer);
    closesocket(sockfd);
    return 0;
}
