cmake_minimum_required(VERSION 3.16)
project(udp_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(.)
FILE(GLOB SRCFILES *.cpp *.hpp)

add_executable(${PROJECT_NAME} main.cpp ${SRCFILES})
target_link_libraries(${PROJECT_NAME} pthread)
