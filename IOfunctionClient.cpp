#include "bits/stdc++.h"
#include <Ws2tcpip.h>

using namespace std;

int main() {
    HANDLE hPipe;
    DWORD bytesWritten;
    for (size_t i = 0; i < 50; ++i) {
        WaitNamedPipe(TEXT("\\\\.\\pipe\\mypip"), NMPWAIT_WAIT_FOREVER);  // 等待pipe就绪
        hPipe = CreateFile(                                              // 连接pipe
                TEXT("\\\\.\\pipe\\mypip"),
                GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);
        if (hPipe != INVALID_HANDLE_VALUE) {                           // 通过pipe发送消息
            string message("hello pip");
            message += to_string(i);
            message += "\n";
            cout << "sending message:" << message << endl;
            WriteFile(hPipe,
                      message.c_str(),
                      message.size(),
                      &bytesWritten,
                      NULL);
            CloseHandle(hPipe);                                         // 关闭pipe
        }
    }
    return 0;
}