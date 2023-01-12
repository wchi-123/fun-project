cmake_minimum_required(VERSION 2.8.4)

project(main)
add_definitions(-Wall -O0 -g)

set(SOURCE_FILES main.c)

include_directories("/usr/local/include" "${PROJECT_SOURCE_DIR}/deps")

link_directories("/usr/local/lib")

add_executable(main ${SOURCE_FILES})
