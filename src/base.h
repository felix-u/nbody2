// 2023-10-02
#ifndef BASE_H
#define BASE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef   uint8_t    u8;
typedef  uint16_t   u16;
typedef  uint32_t   u32;
typedef  uint64_t   u64;
typedef    int8_t    i8;
typedef   int16_t   i16;
typedef   int32_t   i32;
typedef   int64_t   i64;
typedef    size_t usize;
typedef uintptr_t  uptr;
typedef  intptr_t  iptr;
typedef     float   f32;
typedef    double   f64;

typedef  i8  b8;
typedef i16 b16;
typedef i32 b32;
typedef i64 b64;

typedef struct str8 {
    u8 *ptr;
    usize len;
} str8;

#define str8_lit(s) (str8){ .ptr = (u8 *)s, .len = sizeof(s) - 1 }

#define str8_expand(s) (s).ptr, (s).len

#define str8_fmt(s) (int)(s).len, (s).ptr

str8 str8_from_cstr(const char *s);
void str8_overwrite_at_idx(str8 s1, const usize idx, const str8 s2);
void str8_print(FILE *stream, const str8 str);

#include <stdlib.h>

typedef struct Arena {
    void *mem;
    usize offset;
    usize cap;
} Arena;

bool arena_init(Arena *arena);
void _arena_align(Arena *arena, usize align);
void *arena_alloc(Arena *arena, usize size);
void arena_deinit(Arena *arena);

str8 file_read(Arena *arena, str8 path);

#endif // BASE_H


#ifdef BASE_IMPLEMENTATION
#undef BASE_IMPLEMENTATION

str8 str8_from_cstr(const char *s) {
    usize len = 0;
    while (s[len] != '\0') len += 1;
    return (str8){ .ptr = (u8 *)s, .len = len };
}

void str8_overwrite_at_idx(str8 s1, const usize idx, const str8 s2) {
    for (usize i = idx; idx + i < s1.len && i < s2.len; i += 1) {
        s1.ptr[idx + i] = s2.ptr[i];
    }
}

void str8_print(FILE *stream, const str8 str) {
    for (usize i = 0; i < str.len; i++) putc(str.ptr[i], stream);
}

str8 file_read(Arena *arena, str8 path) {
    FILE *fp = NULL;
    const str8 none = { 0 };
    str8 buf = { 0 };

    if (!(fp = fopen((char *)path.ptr, "r"))) return none;
    
    fseek(fp, 0L, SEEK_END);
    usize filesize = ftell(fp);
    if (!(buf.ptr = arena_alloc(arena, filesize + 1))) {
        fclose(fp);
        return none;
    }

    fseek(fp, 0L, SEEK_SET);
    buf.len = fread(buf.ptr, sizeof(u8), filesize, fp);
    buf.ptr[buf.len] = '\0';

    if (ferror(fp)) {
        fclose(fp);
        return none;
    }

    fclose(fp);
    return buf;
}

bool arena_init(Arena *arena) {
    arena->offset = 0;
    if (arena->cap == 0) arena->cap = 8 * 1024 * 1024; 
    return (arena->mem = malloc(arena->cap));
}

void _arena_align(Arena *arena, usize align) {
    usize modulo = arena->offset & (align - 1);
    if (modulo != 0) arena->offset += align - modulo;
}

#define ARENA_DEFAULT_ALIGNMENT (2 * sizeof(void *))

void *arena_alloc(Arena *arena, usize size) {
    _arena_align(arena, ARENA_DEFAULT_ALIGNMENT);
    if (arena->offset >= arena->cap) return NULL;
    void *ptr = (u8 *)arena->mem + arena->offset;
    arena->offset += size;
    return ptr;
}

void arena_deinit(Arena *arena) {
    free(arena->mem);
    arena->offset = 0;
    arena->cap = 0;
}

#endif // BASE_IMPLEMENTATION
