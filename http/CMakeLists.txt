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

project(http)

set(SOURCES 
    ./http_server.cpp
    ./http_utils.cpp
)

add_library(${PROJECT_NAME} 
    STATIC 
    ${SOURCES})

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    include
)

target_compile_options(
    ${PROJECT_NAME}
    PRIVATE
    -stdlib=libc++ 
)

target_link_libraries(${PROJECT_NAME} 
    socket
)


# # Test
# find_package(GTest REQUIRED)

# add_executable(TcpTest tests/test_tcp.cpp ${SOURCES})

# target_include_directories(TcpTest PRIVATE 
#     ${GTEST_INCLUDE_DIRS} 
#     include
# )

# target_link_libraries(TcpTest PRIVATE 
#     ${GTEST_LIBRARIES} 
#     ${PROJECT_NAME}
# )

# add_executable(UdpTest tests/test_udp.cpp ${SOURCES})

# target_include_directories(UdpTest PRIVATE 
#     ${GTEST_INCLUDE_DIRS} 
#     include
# )

# target_link_libraries(UdpTest PRIVATE 
#     ${GTEST_LIBRARIES} 
#     ${PROJECT_NAME}
# )

