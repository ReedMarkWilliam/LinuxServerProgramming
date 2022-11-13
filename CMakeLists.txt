cmake_minimum_required(VERSION 3.23)

project(LinuxServerProgamming)

set(CMAKE_CXX_STANDARD 20)
#link_libraries(ws2_32 wsock32)

#声明头文件路径
set(INC_DIR ./include)
set(INC_DIR2 ./src)

#声明链接库路径
set(LINK_DIR ./lib)

#引入头文件
include_directories(${INC_DIR})
include_directories(${INC_DIR2})
#引入库文件
link_directories(${LINK_DIR})



add_executable(LinuxServerProgamming ChangeTcpSendBuffer.cpp)
add_executable(ChangeTcpReceiveBuffer ChangeTcpReceiveBuffer.cpp)
add_executable(ChangeTcpSendBuffer ChangeTcpSendBuffer.cpp)
add_executable(GetDaytime GetDaytime.cpp)
add_executable(IOfunctionClient IOfunctionClient.cpp)
add_executable(IOfunction IOfunction.cpp)
add_executable(BasicCGIWithDup BasicCGIWithDup.cpp src/myepoll.c)
add_executable(WebReadv WebReadv.cpp)
add_executable(SendMessage SendMessage.cpp)
add_executable(SendFile SendFile.cpp)
add_executable(LinuxGetDayTime LinuxGetDayTime.cpp)
add_executable(ChatClient ChatClient.cpp)
add_executable(ChatServer ChatServer.cpp)
add_executable(main main.cpp)
add_executable(TcpEchoClient TcpEchoClient.cpp)
add_executable(Echoserver Echoserver.cpp)
add_executable(Signal Signal.cpp)
add_executable(SigUrgOutOfBand SigUrgOutOfBand.cpp)
add_executable(SetConnectTimeO SetConnectTimeO.cpp)
add_executable(LST_Timer LST_Timer.cpp)
add_executable(TimeWheel TimeWheel.cpp)
add_executable(TimeHeap TimeHeap.cpp)
add_executable(LibeventHelloWorld LibeventHelloWorld.cpp)


target_link_libraries(LibeventHelloWorld -levent)
#target_link_libraries(${PROJECT_NAME} -levent)

