#include "bits/stdc++.h"
#include <Ws2tcpip.h>
#define BUFFER_SIZE 512

using namespace std;

// 同一台电脑充当客户端和服务端使用抓包工具无法抓到
// 在同一局域网下，树莓派运行Receive，参数为192.168.2.60 12345 2500
// 电脑端运行Send，参数为192.168.2.60 5000，使用wireshark抓包，过滤为ip.dst == 192.168.2.60
// Tcp接收缓冲区最小值每个机器不同，比如树莓派是2304，设置比它小，结果依然是2304，如果比它大，则会翻倍
// Windows发送缓冲区没有变化，与书中描述不相符

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
    server_address.sin_port = port;
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    int sendBuf = atoi(argv[3]);
    int len = sizeof(sendBuf);
    // SOL_SOCKET 通用socket选型
    // SO_SNDBUF TCP发送缓冲区，最小值是2048字节
    // SO_RCVBUF TCP接收缓冲区，最小值是256字节
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&sendBuf, sizeof(sendBuf));
    getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&sendBuf, (socklen_t*)&len);
    printf("the tcp send buffer size after setting is%d\n", sendBuf);

    if (connect(sock, (sockaddr*)&server_address, sizeof(server_address)) != -1) {
        // char buffer[BUFFER_SIZE];
        // memset(buffer, 'a', BUFFER_SIZE);
        char *buffer = "Hello world\r\n";
        send(sock, buffer, strlen(buffer), 0);
    }

//    if (connect(sock, (sockaddr*)&server_address, sizeof(server_address)) != -1) {
//        const char* oob_data = "abc";
//        const char* normal_data = "123";
//        send(sock, normal_data, strlen(normal_data), 0);
//        send(sock, oob_data, strlen(oob_data), MSG_OOB);
//        send(sock, normal_data, strlen(normal_data), 0);
//    }

    // Receive message from server
    char recvBuf[1024] = {0};
    while (recv(sock, recvBuf, sizeof(recvBuf), 0) > 0) {
        cout << "Has received" << '\n';
        cout << recvBuf << '\n';
    }
    closesocket(sock);
    return 0;
}