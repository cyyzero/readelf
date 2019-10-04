cmake_minimum_required(VERSION 3.14)
project(readelf)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)

add_executable(readelf
        src/ELF_reader.cpp
        src/ELF_reader.h
        src/main.cpp)
