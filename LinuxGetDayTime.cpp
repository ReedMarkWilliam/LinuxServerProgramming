#include "bits/stdc++.h"
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

using namespace std;

// 使用方法，输入localhost，Linux下需要安装daytime服务
// sudo apt-get install xinetd
// etc/xinetd.d目录下 sudo vim daytime，将两个disable的值由yes 改为 no
// sudo /etc/init.d/xinetd restart 重启即可

int main(int argc, char* argv[]) {
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
    if (result == -1) {
        perror("opps:getdate");
        exit(1);
    }
    char buffer[128];
    result = recv(sockfd, buffer, sizeof(buffer), 0);
    assert(result > 0);
    buffer[result] = '\0';
    printf("the day time is:%s", buffer);
    close(sockfd);
    return 0;
}
