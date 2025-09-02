#ifndef GAME_PLATFORM_COMUNICATION_H
#define GAME_PLATFORM_COMUNICATION_H

#include "base.h"

// NOTE: this is the idea for file handling for now (August 12 2025, 15:27)
//  Create a func pointer that the game expects the platform to provide
//  Platform then procides he func with that type and passes to the game
//  If not this way then the pl;atform provides a file with code for that type 
//  that then the game directly uses
//  Kinda like a wrapper that results in a platform generic code  
//  Also defining macros for func types may be a good idea, 
//  so then the macro can be used insted of the whole func signature when defining inside a platform

// =========================================================

// TODO: insted of a stack allocated buffer, 
//       pass in an allocator like arena and allocate there
struct File_read_result {
    U32 size;
    U8* data;
};
#define PlatformSpecificReadfileF(func_name) File_read_result func_name (Arena* arena, char* file_name_null_terminated)   
typedef PlatformSpecificReadfileF(platform_read_file_ft);
// NOTE: to not have to put * everywhere for these typedefs, the following might be used
//   --> typedef PlatformSpecificReadfileF((*platform_read_file_ft));

// =========================================================

struct File_write_result {
    U32 bytes_written;
};
#define PlatformSpecificWritefileF(func_name) File_write_result func_name (char* file_name_null_terminated,  \
                                                                           U8* buffer_of_data_to_be_written, \
                                                                           U32 buffer_data_size)   
typedef PlatformSpecificWritefileF(platform_write_file_ft);

// =========================================================

#define PlatformSpecificGetPerfCounter(func_name) U64 func_name ()
typedef PlatformSpecificGetPerfCounter(platform_get_perf_counter);

// =========================================================


struct Bitmap {
    U32 width;
    U32 height;
    
    U32* mem;
    U32 bytes_per_px;
};

struct Memory {
    U32 permanent_mem_size;
    void* permanent_mem;

    U32 frame_mem_size;
    void* frame_mem;
};

struct Keyboard_input {
    B32 is_used;
    B32 w_pressed;
    B32 a_pressed;
    B32 s_pressed;
    B32 d_pressed;
    B32 shift_pressed;    
    B32 q_pressed;
    B32 r_pressed;
    B32 left_arrow_pressed;
};

// TODO: do some with this
struct Some_more_platform_things_to_use {
    platform_read_file_ft* read_file_fp;
    platform_write_file_ft* write_file_fp;    

    platform_get_perf_counter* get_perf_counter;
    U64 perf_counts_per_sec;
};










#endif 