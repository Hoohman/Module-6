cmake_minimum_required(VERSION 3.30)
project(task-1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS json)

add_subdirectory(NetworkLogFilter)
add_subdirectory(receiver_service)