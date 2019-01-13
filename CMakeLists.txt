cmake_minimum_required(VERSION 3.13)
project(RUDP)

set(CMAKE_CXX_STANDARD 14)

# 引入子目录
ADD_SUBDIRECTORY(Client)
ADD_SUBDIRECTORY(Server)