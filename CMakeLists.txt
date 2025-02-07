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
aux_source_directory(./net/application net_application_src)
aux_source_directory(./net/common net_common_src)


add_library(utils STATIC ${utils_src})
add_library(net::utils ALIAS utils)
target_include_directories(utils PUBLIC ./utils/include)

add_library(common STATIC ${net_common_src})
add_library(net::common ALIAS common)
target_include_directories(common PUBLIC ./net/common/include)

add_library(net_socket STATIC ${net_socket_src})
add_library(net::socket ALIAS net_socket)
target_include_directories(net_socket PUBLIC ./net/socket/include)
target_link_libraries(net_socket PUBLIC net::utils net::common)

# add_library(net_application STATIC ${net_application_src})
# add_library(net::application ALIAS net_application)
# target_include_directories(net_application PUBLIC ./net/application/include ${OPENSSL_INCLUDE_DIR})
# target_link_libraries(net_application PUBLIC net::utils net::socket OpenSSL::SSL OpenSSL::Crypto net::common)

add_executable(TcpServerTest ./demo/tcp_server.cpp)
target_link_libraries(TcpServerTest PUBLIC net::utils net::socket)
# add_executable(TcpClientTest ./demo/tcp_client.cpp)
# target_link_libraries(TcpClientTest PUBLIC net::utils net::socket)
# add_executable(HttpServerTest ./demo/http_server.cpp)
# target_link_libraries(HttpServerTest PUBLIC net::utils net::socket net::application)
# add_executable(HttpClientTest ./demo/http_client.cpp)
# target_link_libraries(HttpClientTest PUBLIC net::utils net::socket net::application)
# add_executable(HttpsServerTest ./demo/https_server.cpp)
# target_link_libraries(HttpsServerTest PUBLIC net::utils net::socket net::application)
# add_executable(HttpsClientTest ./demo/https_client.cpp)
# target_link_libraries(HttpsClientTest PUBLIC net::utils net::socket net::application)
# add_executable(WebSocketServerTest ./demo/websocket_server.cpp)
# target_link_libraries(WebSocketServerTest PUBLIC net::utils net::socket net::application)
# add_executable(WebSocketClientTest ./demo/websocket_client.cpp)
# target_link_libraries(WebSocketClientTest PUBLIC net::utils net::socket net::application)

# install headers

install(DIRECTORY ${PROJECT_SOURCE_DIR}/utils/include/ DESTINATION ${INCLUDE_OUTPUT_PATH}/net/utils)
# install(DIRECTORY ${PROJECT_SOURCE_DIR}/net/include/ DESTINATION ${INCLUDE_OUTPUT_PATH}/net/net)


# build test

find_package(GTest REQUIRED)


# utils tests

add_executable(LoggerTest tests/test_logger.cpp)
target_link_libraries(LoggerTest GTest::GTest net::utils)

add_executable(PrintTest tests/test_print.cpp)
target_link_libraries(PrintTest GTest::GTest net::utils)

add_executable(ThreadPoolTest tests/test_thread_pool.cpp)
target_link_libraries(ThreadPoolTest GTest::GTest net::utils)

# add_executable(ParserTest tests/parser_test.cpp)
# target_link_libraries(ParserTest PUBLIC net::utils net::socket net::application GTest::GTest)



