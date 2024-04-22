@echo off

REM set deps=.\deps
REM set dir_rl=%deps%\raylib

REM set libs=%dir_rl%\lib\libraylib.a -I%dir_rl%\include -lgdi32 -lwinmm

REM set cflags=-std=c99 -pedantic ^
REM     -Wall -Werror -Wextra -Wshadow -Wconversion -Wdouble-promotion ^
REM     -Wno-unused-function -Wno-sign-conversion -fno-strict-aliasing ^
REM     -g3 -fsanitize=undefined -fsanitize-trap -DDEBUG

REM zig cc %cflags% -o nbody2.exe src\main.c %libs%

zig build
