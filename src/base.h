#ifndef BASE_H
#define BASE_H

#include <stdint.h>

typedef int8_t S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef float  F32;
typedef double F64;

typedef S32 B32;

#define file_private static
#define global static

#define namespaced_enum enum class

#define Bytes_U32(value)     ((U32) value) 
#define Kilobytes_U32(value) ((U32)1024 * Bytes_U32(value))
#define Megabytes_U32(value) ((U32)1024 * Kilobytes_U32(value))
#define Gigabytes_U32(value) ((U32)1024 * Megabytes_U32(value))
#define Terabytes_U32(value) ((U32)1024 * Gigabytes_U32(value))  

#define Assert(expr) if (!(expr))  { *((U8*)0) = 69; }

#define InvalidCodePath Assert(false);
#define DebugStopHere int x = 69;

// TODO: move this out to its own arena file maybe 
// == These are just some more things, but proably moving them out would make more sense
struct Arena {
    U8* base;
    U32 pos;
    U32 cap;
};
// NOTE: init here means that we are just initing the arena, the memory has alredy been preallocated somewhere else
//       alloc would mean that arena allocated memory for itslef
//       same logic aplies to uninnit and release
Arena arena_init(void* mem, U32 cap);
Arena arena_init(void* mem, U32 cap) {
    Arena result = {};
    result.base = (U8*)mem;
    result.pos  = 0;
    result.cap  = cap;
    return result;
}
void arena_uninnit(Arena* arena);
void arena_uninnit(Arena* arena) {
    arena->base = nullptr;
    arena->pos  = 0;
    arena->cap  = 0;
}
void* arena_push(Arena* arena, U32 size);
void* arena_push(Arena* arena, U32 size) {
    Assert(arena->pos + size < arena->cap);

    void* result = (void*)(arena->base + arena->pos);
    arena->pos += size;

    return result;
}
#define ArenaPush(arena_p, type)           (type*) arena_push(arena_p, sizeof(type))
#define ArenaPushArr(arena_p, type, count) (type*) arena_push(arena_p, sizeof(type) * count)
// ================================================================


// == Arrays
// NOTE: this is here for now
#define ArrayCount(arr) (sizeof(arr) / sizeof(arr[0]))

// TODO: think about this, maybe use arena insted of stack for these arrays, like ryan, so no need for arr[size] needed. Some like that....
#define DefineArrayType(type, name) struct name { \
                                        type* base; \
                                        U64 count; \
                                    };

// ============================================

// == Some fancy Ryan Fleury type macros for data structures
#define StackPush(top_node_p, new_node_p) new_node_p->prev = top_node_p; \
                                          top_node_p = new_node_p;
// ========================================================







#endif