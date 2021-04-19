cmake_minimum_required(VERSION 3.0)
project(Glad)

#set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../)
add_library(glad glad.h glad.c)
target_include_directories(glad PUBLIC .)
