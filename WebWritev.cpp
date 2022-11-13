#include "bits/stdc++.h"
#include <Ws2tcpip.h>
#include "io.h"
#include <sys\stat.h>

using namespace std;

#define BUFFER_SIZE 1024
static const char* status_line[2] = {"200 OK", "500 Internal server error"};

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
    if (argc <= 3) {
        printf("usage:%s ip_address port_number filename\n", argv[0]);
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    const char* file_name = argv[3];
    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = port;
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("socket create failed, error:%d\n", WSAGetLastError());
        return 1;
    }
    int ret = bind(sock, (sockaddr*)&address, sizeof(address));
    if (ret == -1) {
        printf("ret failed, error:%d\n", WSAGetLastError());
        return 1;
    }
    ret = listen(sock, 5);  // backlog = 5表示socket的排队个数，单线程下允许的最大连接为backlog + 1，多线程为backlog + n,n为线程数
    if (ret == -1) {
        printf("listen failed, error:%d\n", WSAGetLastError());
        return 1;
    }
    sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (sockaddr*)&client, &client_addrlength);
    if (connfd < 0) {
        printf("accept failed, error:%d\n", WSAGetLastError());
        return 1;
    } else {
        char header_buf[BUFFER_SIZE];  // 保存Http应答的状态行，头部字段和一个空行的缓冲区
        memset(header_buf, '\0', BUFFER_SIZE);
        char* file_buf;   // 用于存放目标文件内容的应用程序缓存
        struct stat file_stat;   // 用于获取目标文件的属性，比如是否为目录、文件大小等
        bool vaild = true; // 记录目标文件是否是有效文件
        int len = 0;
        if (stat(file_name, &file_stat) < 0) {  // stat在sys\stat.h头文件中
            vaild = false;  // 目标文件不存在
        } else {
            if (S_ISDIR(file_stat.st_mode)) {   // 目标文件是一个目录
                vaild = false;  // 目标文件是一个目录
            } else if(file_stat.st_mode && S_IROTH) {
                // 当前用户有读取目标文件的权限
                // 动态分配缓存区file_buf,并指定其大小为目标文件的大小file_stat.st_size + 1
                // 然后将目标文件读入缓存区file_buf中
                HANDLE fd = CreateFile(file_name,     //指向文件名的指针
                                    GENERIC_READ,  // 访问模式，读
                                    FILE_SHARE_READ, // 共享模式
                                    NULL,   // 指向安全属性的指针
                                    OPEN_ALWAYS, // 如何创建，如果不存在就创建
                                    FILE_ATTRIBUTE_NORMAL, // 文件的属性，默认属性
                                    NULL);         // 用于复制文件句柄
                file_buf = new char[file_stat.st_size + 1];
                memset(file_buf, '\0', file_stat.st_size + 1);
                DWORD dwRead;
                if (ReadFile(fd, file_buf, file_stat.st_size, &dwRead, NULL) < 0) {
                    vaild = false;
                }
            } else {
                vaild = false;
            }
        }
        // 如果目标文件有效，发送正常的Http应答
        if (vaild) {
            ret = snprintf(header_buf, BUFFER_SIZE - 1, "%s%s\r\n", "HTTP/1.1", status_line[0]);
            len += ret;
            ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len, "Content Length:%d\r\n", file_stat.st_size);
            len += ret;
            ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len, "%s", "\r\n");
            ReadFileScatter
        }
    }
}