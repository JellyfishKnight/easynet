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

find_package(OpenSSL REQUIRED)

# set library output path
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(INCLUDE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/include)

message(STATUS "LIBRARY_OUTPUT_PATH: ${INCLUDE_OUTPUT_PATH}")
message(STATUS "LIBRARY_OUTPUT_PATH: ${LIBRARY_OUTPUT_PATH}")
message(STATUS "LIBRARY_OUTPUT_PATH: ${EXECUTABLE_OUTPUT_PATH}")

# build net
aux_source_directory(./utils utils_src)
aux_source_directory(./net/socket net_socket_src)
aux_source_directory(./net/http net_http_src)
aux_source_directory(./net/common net_common_src)
aux_source_directory(./net/websocket net_websocket_src)


add_library(utils STATIC ${utils_src})
add_library(net::utils ALIAS utils)
target_include_directories(utils PUBLIC ./utils/include)

add_library(common STATIC ${net_common_src})
add_library(net::common ALIAS common)
target_include_directories(common PUBLIC ./net/common/include)

add_library(net_socket STATIC ${net_socket_src})
add_library(net::socket ALIAS net_socket)
target_include_directories(net_socket PUBLIC ./net/socket/include)
target_link_libraries(net_socket PUBLIC net::utils)

add_library(net_http STATIC ${net_http_src})
add_library(net::http ALIAS net_http)
target_include_directories(net_http PUBLIC ./net/http/include ${OPENSSL_INCLUDE_DIR})
target_link_libraries(net_http PUBLIC net::utils net::socket OpenSSL::SSL OpenSSL::Crypto)

add_library(net_websocket STATIC ${net_websocket_src})
add_library(net::websocket ALIAS net_websocket)
target_include_directories(net_websocket PUBLIC ./net/websocket/include)
target_link_libraries(net_websocket PUBLIC net::utils net::socket net::http net::common)

add_executable(TcpServerTest ./demo/tcp_server.cpp)
target_link_libraries(TcpServerTest PUBLIC net::utils net::socket)
add_executable(TcpClientTest ./demo/tcp_client.cpp)
target_link_libraries(TcpClientTest PUBLIC net::utils net::socket)
add_executable(HttpServerTest ./demo/http_server.cpp)
target_link_libraries(HttpServerTest PUBLIC net::utils net::socket net::http)
add_executable(HttpClientTest ./demo/http_client.cpp)
target_link_libraries(HttpClientTest PUBLIC net::utils net::socket net::http)
add_executable(HttpsServerTest ./demo/https_server.cpp)
target_link_libraries(HttpsServerTest PUBLIC net::utils net::socket net::http)
add_executable(HttpsClientTest ./demo/https_client.cpp)
target_link_libraries(HttpsClientTest PUBLIC net::utils net::socket net::http)

# install headers

install(DIRECTORY ${PROJECT_SOURCE_DIR}/utils/include/ DESTINATION ${INCLUDE_OUTPUT_PATH}/net/utils)
# install(DIRECTORY ${PROJECT_SOURCE_DIR}/net/include/ DESTINATION ${INCLUDE_OUTPUT_PATH}/net/net)


# build test

find_package(GTest REQUIRED)


# utils tests

add_executable(LoggerTest utils/tests/test_logger.cpp)
target_include_directories(LoggerTest PUBLIC ./utils/include)
target_link_libraries(LoggerTest GTest::GTest utils)

add_executable(PrintTest utils/tests/test_print.cpp)
target_include_directories(PrintTest PUBLIC ./utils/include)
target_link_libraries(PrintTest GTest::GTest utils)

add_executable(ThreadPoolTest utils/tests/test_thread_pool.cpp)
target_include_directories(ThreadPoolTest PUBLIC ./utils/include)
target_link_libraries(ThreadPoolTest GTest::GTest utils)


