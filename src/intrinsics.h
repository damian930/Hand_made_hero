#ifndef INTRINSICS_H
#define INTRINSICS_H

#include "base.h"
#include <math.h>

///////////////////////////////////////////////////////////
// Damian: getting some compiler defined here
//
#define COMPILER_MSVC 0
#ifdef _MSC_VER
    #undef COMPILER_MSVC
    #define COMPILER_MSVC 1
#endif

#include <intrin.h>

///////////////////////////////////////////////////////////
// Damian: Example: 0b100100 -> returns true, idx -> 2 
//
B32 find_idx_of_first_least_significant_set_bit(U32 value, U32* idx) 
{
    #if COMPILER_MSVC
    
    B32 found = _BitScanForward((unsigned long*)idx, value);
    return found;
    
    #else
    
    B32 found = false;
    for (U32 shift=0; shift < 32; ++shift) {
        U32 mask = (1 << shift);
        
        if (value & mask) {
            *idx = shift;
            found = true;
            break;
        }
    }
    return found;

    #endif
}
















#endif




































