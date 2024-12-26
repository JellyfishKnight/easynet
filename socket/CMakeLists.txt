cmake_minimum_required(VERSION 3.22.0)

set(CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set Release during the competition
set(CMAKE_BUILD_TYPE Release)
# set compiler as clang 
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_C_COMPILER "/usr/bin/clang")

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

project(socket)

set(SOURCES 
    ./tcp.cpp
    ./udp.cpp
)

add_library(${PROJECT_NAME} 
    STATIC 
    ${SOURCES})

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    include
)

# Test
find_package(GTest REQUIRED)

add_executable(TcpTest tests/test_tcp.cpp)

target_link_libraries(TcpTest PUBLIC 
    ${GTEST_LIBRARIES} 
    socket
)


add_executable(UdpTest tests/test_udp.cpp )

target_link_libraries(UdpTest PUBLIC 
    ${GTEST_LIBRARIES} 
    socket
)


