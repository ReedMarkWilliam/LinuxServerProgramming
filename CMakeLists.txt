cmake_minimum_required(VERSION 3.23)

project(LinuxServerProgamming)

set(CMAKE_CXX_STANDARD 20)
link_libraries(ws2_32 wsock32)

add_executable(LinuxServerProgamming ChangeTcpSendBuffer.cpp)
add_executable(ChangeTcpReceiveBuffer ChangeTcpReceiveBuffer.cpp)
add_executable(ChangeTcpSendBuffer ChangeTcpSendBuffer.cpp)