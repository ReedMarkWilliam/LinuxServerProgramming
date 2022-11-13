#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<signal.h>
#include<fcntl.h>

#define BUF_SIZE 1024
static int conn_fd;

// 使用SIGURG信号接收带外数据时，需要在发送数据前睡眠1s，等待服务器把准备工作做好。
// 客户端在建立连接后如果立即发送，发送完就退出，服务端检测到后也会释放资源退出，会导致以下两种情况：
// 1.当带外数据到达时，SIGURG信号的捕捉函数还没注册上，服务端采用的是默认处理动作，即忽略。
// 2.带外数据还没到达时，客户端已经往发送缓冲区写完数据，然后关闭连接了，导致服务端检测到对端关闭然后退出了

void sig_urg(int sig) {
    int save_error = errno;
    char buf[BUF_SIZE];
    memset(buf, '\0', BUF_SIZE);
    int ret = recv(conn_fd, buf, BUF_SIZE - 1, MSG_OOB);
    printf("got %d bytes if oob data '%s'\n", ret, buf);
    errno = save_error;
}

void add_sig(int sig, void(*sig_handler)(int)) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof sa);
    sa.sa_handler = sig_handler;  // 新的信号处理函数
    sa.sa_flags |= SA_RESTART; // 如果信号中断了进程的某个系统调用，则系统自动启动该系统调用
    sigfillset(&sa.sa_mask);      // 将sa_mask 指定的信号集搁置
    // 检查或修改与指定信号相关联的处理动作
    // sig: 要捕获的信号类型， sa: 指定行的信号处理方式
    assert(sigaction(sig, &sa, NULL) != -1);
}

int main(int argc, char* argv[]) {
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    struct sockaddr_in address;
    bzero(&address, sizeof address);
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_port);
    address.sin_port = htons(static_cast<short>(port));
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    int ret = bind(sock, reinterpret_cast<sockaddr*>(&address), sizeof address);
    assert(ret >= 0);
    ret = listen(sock, 5);
    assert(ret != -1);
    struct sockaddr_in client;
    socklen_t client_len = sizeof client;
    while (1) {}
    //conn_fd = accept(sock, reinterpret_cast<sockaddr*>(&client), &client_len);
    if (conn_fd < 0) {
        printf("errno is %d\n", errno);
        return 1;
    } else {
        add_sig(SIGURG, sig_urg);
        // 使用SIGURG信号前，必须设置socket的宿主进程或进程组
        fcntl(conn_fd, F_SETOWN, getpid());
        char buf[BUF_SIZE];
        while (1) {
            memset(buf, '\0', BUF_SIZE);
            ret = recv(conn_fd, buf, BUF_SIZE - 1, 0);
            if (ret <= 0) {
                break;
            }
            printf("got %d bytes of normal data '%s'\n", ret, buf);
        }
        close(conn_fd);
    }
    close(sock);
}