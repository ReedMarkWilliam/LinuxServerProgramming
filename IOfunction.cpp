#include "bits/stdc++.h"
#include <Ws2tcpip.h>

using namespace std;

// 进程是资源分配的最小单位，线程是资源调度的最小单位
// 进程有自己的独立地址空间，每启动一个进程，系统就会为它分配地址空间，建立数据表来维护代码段、堆栈段和数据段
// 线程是共享进程中的数据的，使用相同的地址空间
// 线程之间通信更方便，同一进程下的线程共享全局变量、静态变量等数据，而进程之间的通信需要以IPC进行


// A pipe is a section of shared memory that processes use for communication.
// The process that creates a pipe is the pipe server.
// A process that connects to a pipe is a pipe client.
// One process writes information to the pipe,
// then the other process reads the information from the pipe.
// 管道：用于进程间通信的共享内存区域
// 创建管道的是管道服务器,连接管道的是客户端
// Windows系统由两种管道：匿名管道（anonymous pipes），只能本地使用，基于字符，半双工
// 命名管道（Named Pipes）

int main() {
    HANDLE hPipe;
    char buffer[1024];
    DWORD readCount;  // unsigned int
    hPipe = CreateNamedPipe(
            TEXT("\\\\.\\pipe\\mypip"),    // 管道名称，格式限定[\\.\pipe\pipe_name]
            PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,  // 可读可写，不允许多个实例
            PIPE_WAIT | PIPE_TYPE_BYTE | PIPE_READMODE_BYTE, //  管道属性，阻塞，字节管道，以字节读取
            1,                // 只能有一个管道实例
            1024,             // 发送缓冲区大小
            1024,             // 接收缓冲区大小
            0,                // 超时，设置为0，但仍有50ms
            NULL
            );
    while (hPipe != INVALID_HANDLE_VALUE) {
        cout << "start connecting..." << endl;
        if (ConnectNamedPipe(hPipe, NULL) != FALSE) {
            cout << "connect-pipe-pass" << endl;
            while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &readCount, NULL) != FALSE) {
                cout << "read-pass" << endl;
                buffer[readCount] = '\0';
                cout << "received:" << buffer << endl;
            }
        }
        DisconnectNamedPipe(hPipe);
    }
    return 0;
}