cmake_minimum_required(VERSION 3.22.0)

set(CXX_STANDARD 20)
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

project(http)


set(SOURCES 
    ./http_server.cpp
    ./http_client.cpp
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

target_link_libraries(${PROJECT_NAME} PUBLIC 
    socket
)


# Test
find_package(GTest REQUIRED)

add_executable(HttpTest tests/http_test.cpp ${SOURCES})

target_link_libraries(HttpTest PUBLIC 
    ${GTEST_LIBRARIES} 
    socket
)

target_include_directories(HttpTest PUBLIC 
    ${GTEST_INCLUDE_DIRS} 
    include
)
