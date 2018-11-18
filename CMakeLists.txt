cmake_minimum_required(VERSION 3.12)
project(CS3026_Assessment C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(CS3026_Assessment
        filesys.c
        filesys.h shell.c)
