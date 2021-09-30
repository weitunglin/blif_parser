cmake_minimum_required(VERSION 3.14)

project(blif_parser)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(blif_parser blif_parser.cpp)