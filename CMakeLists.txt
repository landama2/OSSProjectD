cmake_minimum_required(VERSION 3.1)
project(OSSProjectD)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c99")

set(SOURCE_FILES test_cfg_parser.c route_cfg_parser.c route_cfg_parser.h routing.cfg main.c main.h)
add_executable(OSSProjectD ${SOURCE_FILES} main.c)