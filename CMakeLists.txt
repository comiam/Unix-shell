cmake_minimum_required(VERSION 3.15)
project(unix_shell C)

set(CMAKE_C_STANDARD 99)

add_executable(unix_shell shell.c shell.h promptline.c promptline.h)