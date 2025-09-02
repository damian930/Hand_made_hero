// TODO: remove these 
#include "stdio.h"
#include <fstream>
#include <string>

#include "windows.h"
#include "base.h"
#include "game_platform_comunication.h"
#include "august.cpp"

// == File system, might get moved out at some point, here for now

// NOTE: Maybe opning a file and givingthe handle to the user would be better. 
PlatformSpecificReadfileF(DEBUG_win32_read_file) {
    // TODO: handle longer path better, read docs for CreateFile lpFileName
    File_read_result result = {};

    HANDLE file_handle = CreateFileA(file_name_null_terminated,
                                     GENERIC_READ | GENERIC_WRITE,
                                     0, 
                                     NULL,
                                     OPEN_EXISTING,    
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL);
    if (file_handle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER file_size;
        if (GetFileSizeEx(file_handle, &file_size)) 
        {
            // Alloc memory
            result.data = ArenaPushArr(arena, U8, file_size.QuadPart);
            
            // NOTE: LAEGE_INTEGER.QuadPart is a U64, but ReadFIle takes in a U32 as the bytes_to_read value.
            //       this is bad. Deal with it. Cause U32 is not that much for big files.
            U32 bytes_read;
            if (ReadFile(file_handle, 
                         result.data, 
                         file_size.QuadPart, 
                         (DWORD*)&bytes_read, 
                         NULL)) 
            {
                Assert(bytes_read == file_size.QuadPart);
                result.size = bytes_read;
            }
            else 
            {
                // TODO: handle
                Assert(false);
            }
        }
        else 
        {
            // TODO: handle
            Assert(false);
        }

        CloseHandle(file_handle);
    }
    else {
        // TODO: handle
        Assert(false);
    }
    
    return result;
}

PlatformSpecificWritefileF(DEBUG_win32_write_file) {
    // TODO: handle longer path better, read docs for CreateFile lpFileName
    File_write_result result = {};

    HANDLE file_handle = CreateFileA(file_name_null_terminated,
                                     GENERIC_READ | GENERIC_WRITE,
                                     0, 
                                     NULL,
                                     CREATE_ALWAYS,    
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL);
    if (file_handle != INVALID_HANDLE_VALUE)
    {   
        if (WriteFile(file_handle, 
                      (void*) buffer_of_data_to_be_written, 
                      buffer_data_size, 
                      (DWORD*)&result.bytes_written, 
                      NULL)) 
        {
            result = result;
        }
        else {
            // TODO: handle
            Assert(false);
        }

        CloseHandle(file_handle);
    }
    else
    {
        // TODO: handle
        Assert(false);
    }

    return result;
}

// ===============================================================

struct Win32_bitmap {
    U32 width;
    U32 height;
    U32 bytes_per_px;
    void* mem;

    BITMAPINFO win32_info;
};

// TODO: remove these globals
// NOTE: global bitmap cant be removed, since it is needed for the win32 callback handler.
//       but moving it into main like mnk key inputs is not possible, 
//       since callback is still called from somewhere outside of DipatchMessage
global B32 running        = true;
global B32 is_black       = true;
global Win32_bitmap bitmap = {}; 

file_private void init_or_resize_bitmap(U32 width, U32 height) {
    if (bitmap.mem) {
        B32 err = VirtualFree(bitmap.mem, 0, MEM_RELEASE);
        bitmap.mem = nullptr;
        Assert(err);
    }

    BITMAPINFOHEADER bitmap_header = {};
    bitmap_header.biSize = sizeof(BITMAPINFOHEADER);
    bitmap_header.biWidth = width;
    bitmap_header.biHeight = -height; // NOTE: Negative makes it be top_down from the top-left corner
    bitmap_header.biPlanes = 1;
    bitmap_header.biBitCount = 4 * 8;
    bitmap_header.biCompression = BI_RGB;
    bitmap_header.biSizeImage = NULL;
    
    bitmap.win32_info = {};
    bitmap.win32_info.bmiHeader = bitmap_header;

    bitmap.width = width;
    bitmap.height = height;
    bitmap.bytes_per_px = bitmap_header.biBitCount / 8;
    
    if (width != 0 || height != 0) {
        bitmap.mem = VirtualAlloc(NULL, 4 * width * height, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        Assert(bitmap.mem);
    }
    Assert(bitmap.bytes_per_px == 4);

} 

file_private void update_window(HDC device_context, RECT device_rect) {
    U32 width = device_rect.right - device_rect.left;
    U32 height = device_rect.bottom - device_rect.top;

    StretchDIBits(device_context, 
                  0, 0,  width, height, 
                  0, 0, bitmap.width, bitmap.height, 
                  bitmap.mem, 
                  &bitmap.win32_info, 
                  DIB_RGB_COLORS, 
                  SRCCOPY);
}

// TODO: Assert all the B32 return from win32, so nothing important is skipped 

LRESULT win32_window_proc_handler(HWND    window_handle,
                                  UINT    message,
                                  WPARAM  w_param,
                                  LPARAM  l_param
) {
    LRESULT result = 0;

    switch (message) 
    {
        case WM_DESTROY: {
            running = false;
        } break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
			HDC context = BeginPaint(window_handle, &ps);

            HDC device_context = GetDC(window_handle);
            RECT window_rect;
            GetClientRect(window_handle, &window_rect);
            update_window(device_context, window_rect);
            ReleaseDC(window_handle, device_context);

            EndPaint(window_handle, &ps);
        } break;

        case WM_SIZE: {
            U32 width = 0;
            U32 height = 0;
            {
                RECT window_rect;
                GetClientRect(window_handle, &window_rect);
                width = window_rect.right - window_rect.left;
                height = window_rect.bottom - window_rect.top;
            }

            init_or_resize_bitmap(width, height);
        } break;

        case WM_SYSKEYUP:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_KEYDOWN: {
            Assert(false);
        } break;

        default: {
            result = DefWindowProc(window_handle, message, w_param, l_param);
        } break;
    }

    return result;
}

// ================================================================
void construct_keyboard_input_for_the_current_frame(MSG* message, Keyboard_input* input) {
    // w_param is the key pressed
    // l_param hold some extra infor about the key 
    
    // NOTE: != 0 is used for was down, since we cant use == becasue when we check for 0b0 value insted of some 0b001000 value
    U8 was_down = ((message->lParam & (1 << 30)) != 0);
    U8 is_down  = ((message->lParam & (1 << 31)) == 0);

    if (message->wParam == 'W') {
        if (was_down != is_down) {
            input->w_pressed = is_down;
        }
    }  
    if (message->wParam == 'A') {
        if (was_down != is_down) {
            input->a_pressed = is_down;
        }
    }
    if (message->wParam == 'S') {
        if (was_down != is_down) {
            input->s_pressed = is_down;
        }
    }
    if (message->wParam == 'D') {
        if (was_down != is_down) {
            input->d_pressed = is_down;
        }
    }
    if (message->wParam == VK_SHIFT) {
        if (was_down != is_down) {
            input->shift_pressed = is_down;
        }
    }
    if (message->wParam == 'Q') {
        if (was_down != is_down) {
            input->q_pressed = is_down;
        }
    }
    if (message->wParam == 'R') {
        if (was_down != is_down) {
            input->r_pressed = is_down;
        }
    }
    if (message->wParam == VK_LEFT) {
        if (was_down != is_down) {
            input->left_arrow_pressed = is_down;
        }
    }

    input->is_used = (input->w_pressed     || 
                      input->a_pressed     || 
                      input->s_pressed     || 
                      input->d_pressed     || 
                      input->shift_pressed ||
                      input->q_pressed     || 
                      input->r_pressed     ||
                      input->left_arrow_pressed
                    );
}
// ================================================================

U64 get_perf_counter() {
    LARGE_INTEGER large_int = {};
    B32 err = QueryPerformanceCounter(&large_int);
    Assert(err);
    return large_int.QuadPart;
}

#if DEBUG_MODE
global char file_with_frame_time_data_path[] = "file_with_frame_times.txt";
global std::fstream file_with_frame_times(file_with_frame_time_data_path, std::ios::in | std::ios::out | std::ios::trunc);
#endif

int WINAPI WinMain(HINSTANCE h_instance, 
                   HINSTANCE h_prev_instance,
                   PSTR cmd_attr, 
                   int window_show_attr
) {
    WNDCLASSA window_class = {};
    window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    window_class.lpszClassName = "August";
    window_class.hInstance = h_instance;
    window_class.lpfnWndProc = win32_window_proc_handler;

    ATOM class_atom = RegisterClassA(&window_class);
    if (class_atom) 
    {
        HWND window_handle = CreateWindowExA(NULL, 
                                             window_class.lpszClassName, 
                                             "August", 
                                             WS_VISIBLE | WS_OVERLAPPEDWINDOW,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             1400, 
                                             700, 
                                             NULL, 
                                             NULL,
                                             h_instance, 
                                             NULL);
        if (window_handle)
        {
            Memory game_mem = {};
            
            game_mem.permanent_mem_size = Gigabytes_U32(2);
            game_mem.permanent_mem = VirtualAlloc(NULL, game_mem.permanent_mem_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            Assert(game_mem.permanent_mem);
            
            game_mem.frame_mem_size = Megabytes_U32(50);
            game_mem.frame_mem = VirtualAlloc(NULL, game_mem.frame_mem_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            Assert(game_mem.frame_mem);

            Keyboard_input keyboard_input = {};

            U64 perf_counter_per_sec;
            {
                LARGE_INTEGER large_int = {};
                B32 err = QueryPerformanceFrequency(&large_int);
                Assert(err);
                perf_counter_per_sec = large_int.QuadPart;
            }

            U32 fps = 30;
            F32 max_sec_per_frame = 1.0f/fps;

            while(running) 
            {
                U64 frame_start_counter = get_perf_counter();

                MSG message;
                while(PeekMessage(&message, window_handle, NULL, NULL, PM_REMOVE))
                {   
                    if (message.message == WM_SYSKEYUP   || 
                        message.message == WM_SYSKEYDOWN ||
                        message.message == WM_KEYUP      ||
                        message.message == WM_KEYDOWN
                    ) {
                        construct_keyboard_input_for_the_current_frame(&message, &keyboard_input);
                    }
                    else {
                        //TranslateMessage(&message); 
                        DispatchMessageA(&message);

                    }
                }

                // Update the bitmap here
                {
                    Bitmap game_bitmap = {};
                    game_bitmap.width        = bitmap.width;
                    game_bitmap.height       = bitmap.height; 
                    game_bitmap.mem          = (U32*)bitmap.mem;
                    game_bitmap.bytes_per_px = bitmap.bytes_per_px;

                    Some_more_platform_things_to_use platform_things = {};
                    platform_things.read_file_fp  = DEBUG_win32_read_file;
                    platform_things.write_file_fp = DEBUG_win32_write_file;

                    game_update(&game_bitmap, &game_mem, &keyboard_input, &platform_things, max_sec_per_frame);
                }

                // Enforcing frame rate
                U64 frame_counter = get_perf_counter();
                F32 frame_time = (F32)(frame_counter - frame_start_counter) / perf_counter_per_sec;
                while (frame_time < max_sec_per_frame) {
                    frame_counter = get_perf_counter();
                    frame_time = (F32)(frame_counter - frame_start_counter) / perf_counter_per_sec; 
                }

                // Update the window
                {
                    HDC device_context = GetDC(window_handle);
                    RECT window_rect;
                    GetClientRect(window_handle, &window_rect);
                    update_window(device_context, window_rect);
                    ReleaseDC(window_handle, device_context);
                }
                
                #if DEBUG_MODE
                // Gettimg some debug info for the enforced frame rate
                {
                    Assert(file_with_frame_times.is_open());                    
                    file_with_frame_times << std::to_string(frame_time) << "\n" << std::endl;
                }
                #endif
    
            }

        }
        else {
            // TODO: handle
        }
    }
    else {
        // TODO: handle
    }

    #if DEBUG_MODE
    Assert(file_with_frame_times.is_open());                    
    file_with_frame_times.close();
    #endif


    return 0;
}





























