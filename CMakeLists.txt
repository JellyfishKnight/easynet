cmake_minimum_required(VERSION 3.22.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set Debug during the competition
set(CMAKE_BUILD_TYPE Debug)
# set compiler as clang
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_C_COMPILER "/usr/bin/clang")

message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")

add_compile_options(-Wall -Wextra -Wpedantic)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++ -fexperimental-library")
endif ()

project(net)

# set library output path
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(INCLUDE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/include)

message(STATUS "LIBRARY_OUTPUT_PATH: ${INCLUDE_OUTPUT_PATH}")
message(STATUS "LIBRARY_OUTPUT_PATH: ${LIBRARY_OUTPUT_PATH}")
message(STATUS "LIBRARY_OUTPUT_PATH: ${EXECUTABLE_OUTPUT_PATH}")


# build utils
aux_source_directory(./utils utils_src)

add_library(
  utils STATIC
  ${utils_src}
        utils/include/thread_pool.hpp
)

target_include_directories(
  utils PUBLIC
  ./utils/include
)

# build net
aux_source_directory(./net net_src)

add_library(
  net STATIC
  ${net_src}
)

target_include_directories(
  net PUBLIC
  ./net/include
)

target_link_libraries(
  net
  utils
)

# install headers

install(DIRECTORY ${PROJECT_SOURCE_DIR}/utils/include/ DESTINATION ${INCLUDE_OUTPUT_PATH}/net/utils)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/net/include/ DESTINATION ${INCLUDE_OUTPUT_PATH}/net/net)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/http/include/ DESTINATION ${INCLUDE_OUTPUT_PATH}/net/http)


# build test

find_package(GTest REQUIRED)

add_executable(
  TcpTest
  net/tests/test_tcp.cpp
)

target_link_libraries(
  TcpTest
  GTest::GTest
  net
  utils
)

add_executable(
  UdpTest
  net/tests/test_udp.cpp
)

target_link_libraries(
  UdpTest
  GTest::GTest
  net
  utils
)

add_executable(
  HttpTest
  net/tests/http_test.cpp
)

target_link_libraries(
  HttpTest
  GTest::GTest
  http
  utils
  net
)

add_executable(
  LoggerTest
  utils/tests/test_logger.cpp
)

target_include_directories(
  LoggerTest PUBLIC
  ./utils/include
)

target_link_libraries(
  LoggerTest
  GTest::GTest
  utils
)

add_executable(
  PrintTest
  utils/tests/test_print.cpp
)

target_include_directories(
  PrintTest PUBLIC
  ./utils/include
)

target_link_libraries(
  PrintTest
  GTest::GTest
  utils
)

add_executable(
  ThreadPoolTest
  utils/tests/test_thread_pool.cpp
)

target_include_directories(
  ThreadPoolTest PUBLIC
  ./utils/include
)

target_link_libraries(
  ThreadPoolTest
  GTest::GTest
  utils
)




# build custom client and server

add_executable(
  tcp_client
  demo/http_client.cpp
)

target_link_libraries(
  tcp_client
  utils
  net
  http
  )

add_executable(
  tcp_server
  demo/http_server.cpp
)

target_link_libraries(
  tcp_server
  net
  utils
  http
)

