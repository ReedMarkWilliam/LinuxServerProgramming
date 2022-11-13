#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<stdlib.h>
#include<assert.h>
#include<errno.h>

// 需要打开UDP防火墙

char* s_gets(char* st, int n) {
    char* ret_val;
    int i = 0;
    ret_val = fgets(st, n, stdin);
    if (ret_val) {
        while (st[i] != '\n' && st[i] != '\0') ++i;
        if (st[i] == '\n') st[i] = '\0';
        else {
            while (getchar() != '\n') continue;
        }
    }
    return ret_val;
}

int main(int argc, char* argv[]) {
    if (argc <= 2) {
        printf("usage:%s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    sockaddr_in server_addr;
    socklen_t len = sizeof server_addr;
    bzero(&server_addr, sizeof server_addr);
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(static_cast<short>(port));
    int sockFd = socket(PF_INET, SOCK_DGRAM, 0);
    assert(sockFd >= 0);

    char buf[1024];
    while (1) {
        printf("client:> ");
        s_gets(buf, 1024);
        int ret = sendto(sockFd, buf, strlen(buf), 0, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
        if (ret < 0) {
            printf("sendto error no: %d %d %s\n", ret, errno, strerror(errno));
        }
        if ((recvfrom(sockFd, buf, strlen(buf), 0, reinterpret_cast<sockaddr*>(&server_addr), &len)) < 0) {
            break;
        }
        printf("server:> %s\n", buf);
    }
    close(sockFd);
}