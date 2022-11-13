#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/uio.h>

using namespace std;

#define BUFFER_SIZE 1024
static const char* status_line[2] = {"200 OK", "500 Internal server error"};

int main(int argc, char* argv[]) {
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
        printf("socket create failed, error:%d\n", errno);
        return 1;
    }
    int ret = bind(sock, (sockaddr*)&address, sizeof(address));
    if (ret == -1) {
        printf("bind failed, error:%d\n", errno);
        return 1;
    }
    ret = listen(sock, 5);  // backlog = 5表示socket的排队个数，单线程下允许的最大连接为backlog + 1，多线程为backlog + n,n为线程数
    if (ret == -1) {
        printf("listen failed, error:%d\n", errno);
        return 1;
    }
    sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (sockaddr*)&client, &client_addrlength);
    if (connfd < 0) {
        printf("accept failed, error:%d\n", errno);
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
                int fd = open(file_name, O_RDONLY);
                file_buf = new char[file_stat.st_size + 1];
                memset(file_buf, '\0', file_stat.st_size + 1);
                if (read(fd, file_buf, file_stat.st_size) < 0) {
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
            struct iovec iv[2];
            iv[0].iov_base = header_buf;
            iv[0].iov_len = strlen(header_buf);
            iv[1].iov_base = file_buf;
            iv[1].iov_len = file_stat.st_size;
            ret = writev(connfd, iv, 2);
        } else {
            ret = snprintf(header_buf, BUFFER_SIZE - 1, "%s%s\r\n", "HTTP/1.1", status_line[1]);
            len += ret;
            ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len, "%s", "\r\n");
            send(connfd, header_buf, strlen(header_buf), 0);
        }
        close(connfd);
        delete[] file_buf;
    }
    close(sock);
    return 0;
}






