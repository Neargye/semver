cmake_minimum_required(VERSION 3.6.4)
project(semver_example)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(../src)

set(SRC
    example.cpp
    ../src/semver.hpp)

add_executable(${PROJECT_NAME} ${SRC})