cmake_minimum_required(VERSION 3.15)
project(unix_shell C)

set(CMAKE_C_STANDARD 99)

set (CMAKE_C_FLAGS "-g3")

add_executable(unix_shell shell.c shell.h promptline.c promptline.h dirs.h cmds.c cmds.h dirs.c jobs.c jobs.h signals.c signals.h)