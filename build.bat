@echo off

set cc-command=zig cc ^
    -std=c99 ^
    -Wall -Wextra -pedantic -Werror -Wshadow ^
	-fno-strict-aliasing -Wstrict-overflow

set debug-flags=-g3 -gcodeview -fsanitize=address,undefined -lasan

set release-flags=-O3 -s

%cc-command% %debug-flags% src\main.c -o nbody2.exe
