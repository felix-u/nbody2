@echo off

set deps=.\deps
set dir_rl=%deps%\raylib

set libs=%dir_rl%\lib\libraylib.a -I%dir_rl%\include -lgdi32 -lwinmm

set cflags=-std=c99 -pedantic ^
    -Wall -Werror -Wextra -Wshadow -Wconversion -Wdouble-promotion ^
    -Wno-unused-function -Wno-sign-conversion -fno-strict-aliasing ^
    -g3 -fsanitize=undefined -fsanitize-trap -DDEBUG

zig cc %cflags% -o nbody2.exe src\main.c %libs%
