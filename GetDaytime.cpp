#include "bits/stdc++.h"
#include <Ws2tcpip.h>

using namespace std;

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

}
