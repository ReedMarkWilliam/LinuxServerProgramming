#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<stdlib.h>
#include<assert.h>
#include<fcntl.h>

static int pipeFd[2];


int setNonblocking(int fd) {
    int oldOption = fcntl(fd, F_GETFL);
    int newOption = oldOption | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOption);
    return oldOption;
}

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
    bzero(&server_addr, sizeof server_addr);
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(static_cast<short>(port));
    int sockFd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockFd >= 0);
    if (connect(sockFd, reinterpret_cast<sockaddr*>(&server_addr), sizeof server_addr) < 0) {
        perror("connect");
        return 1;
    }

    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipeFd);
    assert(ret != -1);
    setNonblocking(pipeFd[1]);
    char buf[1024];
    while (1) {
        printf("client:> ");
        s_gets(buf, 1024);
        write(pipeFd[1], (void*)buf, strlen(buf));
        if ((read(sockFd, (void*)buf, sizeof buf)) <= 0) {
            break;
        }
        printf("server:> %s\n", buf);
    }
    close(sockFd);
}
















