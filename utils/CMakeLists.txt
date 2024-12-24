cmake_minimum_required(VERSION 3.22.0)

set(CXX_STANDARD_REQUIRED ON)
set(CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set Release during the competition
set(CMAKE_BUILD_TYPE Release)
# set compiler as clang 
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_C_COMPILER "/usr/bin/clang")

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

project(utils)

add_library(
    ${PROJECT_NAME} STATIC
    thread_pool/thread_pool.cpp
    logger/logger.cpp
)

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