cmake_minimum_required(VERSION 3.22.0)

set(CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set Debug during the competition
set(CMAKE_BUILD_TYPE Debug)
# set compiler as clang 
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_C_COMPILER "/usr/bin/clang")

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

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
)

target_include_directories(
  utils PUBLIC
  ./utils/include
)

# build socket
aux_source_directory(./socket socket_src)

add_library(
  socket STATIC
  ${socket_src}
)

target_include_directories(
  socket PUBLIC
  ./socket/include
)

target_link_libraries(
  socket
  utils
)

# build http

aux_source_directory(./http http_src)

add_library(
  http STATIC
  ${http_src}
)

target_include_directories(
  http PUBLIC
  ./http/include
)

target_link_libraries(
  http
  utils
  socket
)


# install headers

install(DIRECTORY ${PROJECT_SOURCE_DIR}/utils/include/ DESTINATION ${INCLUDE_OUTPUT_PATH})
install(DIRECTORY ${PROJECT_SOURCE_DIR}/socket/include/ DESTINATION ${INCLUDE_OUTPUT_PATH})
install(DIRECTORY ${PROJECT_SOURCE_DIR}/http/include/ DESTINATION ${INCLUDE_OUTPUT_PATH})


# build test

find_package(GTest REQUIRED)

add_executable(
  TcpTest
  socket/tests/test_tcp.cpp
)

target_link_libraries(
  TcpTest
  GTest::GTest
  socket
  utils
)

add_executable(
  UdpTest
  socket/tests/test_udp.cpp
)

target_link_libraries(
  UdpTest
  GTest::GTest
  socket
  utils
)

add_executable(
  HttpTest
  http/tests/http_test.cpp
)

target_link_libraries(
  HttpTest
  GTest::GTest
  http
  utils
  socket
)

# build custom client and server

add_executable(
  tcp_client
  demo/http_client.cpp
)

target_link_libraries(
  tcp_client
  utils
  socket
  http
  )

add_executable(
  tcp_server
  demo/http_server.cpp
)

target_link_libraries(
  tcp_server
  socket
  utils
  http
)

