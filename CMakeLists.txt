cmake_minimum_required(VERSION 3.15)
project(unix_shell C)

set(CMAKE_C_STANDARD 99)

set (CMAKE_C_FLAGS "-std=c11 -lncurses -g3 -Wall -Wextra -Wpedantic -Wunused -Wconversion -D_POSIX_C_SOURCE=200809L")

add_executable(unix_shell shell.c shell.h promptline.c promptline.h dirs.h cmds.c cmds.h dirs.c jobs.c jobs.h signals.c signals.h)