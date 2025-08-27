// TODO: remove this
#pragma once 
#include <math.h>

#include "base.h"

// == Some regular macros
#define Max(x, y) (x > y ? x : y)
#define Min(x, y) (x < y ? x : y)

#define Square(x) ((x) * (x))
#define Abs(x) (x < 0 ? -x : x)

// =======================

// TODO: maybe think about this later
// == Macros for default vector implementations
#define CreateVec2Type(type) struct Vec2_##type { \
                                 type x;          \
                                 type y;          \
                             }

// =======================

struct Vec2_F32 {
    F32 x;
    F32 y;
};
Vec2_F32 vec2_f32(F32 x, F32 y) {
    Vec2_F32 result = {};
    result.x = x;
    result.y = y;
    return result;
}
Vec2_F32 vec2_f32(F32 x) {
    Vec2_F32 result = {};
    result.x = x;
    result.y = x;
    return result;
}

struct Vec2_S32 {
    S32 x;
    S32 y;
};
Vec2_S32 vec2_s32(S32 x, S32 y) {
    Vec2_S32 result = {};
    result.x = x;
    result.y = y;
    return result;
}
Vec2_S32 vec2_s32(S32 x) {
    Vec2_S32 result = {};
    result.x = x;
    result.y = x;
    return result;
}

union Vec3_U32 {
    struct {
        U32 x;
        U32 y;
        U32 z;
    };

    struct {
        U32 r;
        U32 g;
        U32 b;
    };
};

Vec3_U32 vec3_u32(U32 x, U32 y, U32 z) {
    Vec3_U32 result = {};
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
};

// ============



Vec2_F32 operator+(Vec2_F32 v1, Vec2_F32 v2) {
    Vec2_F32 result = {};
    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    return result;
}

Vec2_F32& operator+=(Vec2_F32& v1, Vec2_F32 v2) {
    v1 = v1 + v2;
    return v1;
}

Vec2_F32 operator+(Vec2_F32 v, F32 f) {
    Vec2_F32 result = {};
    result.x = v.x + f;
    result.y = v.y + f;
    return result;
}

Vec2_F32& operator+=(Vec2_F32& v, F32 f) {
    v = v + f;
    return v;
}

Vec2_F32 operator*(F32 f, Vec2_F32 v) {
    Vec2_F32 result = {};
    result.x = v.x * f;
    result.y = v.y * f;
    return result;
}

Vec2_F32 operator*(Vec2_F32 v, F32 f) {
    Vec2_F32 result = {};
    result.x = v.x * f;
    result.y = v.y * f;
    return result;
}

Vec2_F32& operator*=(Vec2_F32& v, F32 f) {
    v.x *= f;
    v.y *= f;
    return v;
}

Vec2_F32 operator*(Vec2_F32 v1, Vec2_F32 v2) {
    Vec2_F32 result = {};
    result.x = v1.x * v2.x;
    result.y = v1.y * v2.y;
    return result;
}

Vec2_F32 operator-(Vec2_F32 v1, Vec2_F32 v2) {
    Vec2_F32 result = {};
    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    return result;
}

Vec2_F32& operator-=(Vec2_F32& v1, Vec2_F32 v2) {
    v1.x -= v2.x;
    v1.y -= v2.y;
    return v1;
}

Vec2_F32 operator-(Vec2_F32 v, F32 f) {
    Vec2_F32 result = v;
    result.x -= f;
    result.y -= f;
    return result;
}

Vec2_F32 operator/(Vec2_F32 v, F32 f) {
    Vec2_F32 result = {};
    result.x = v.x / f;
    result.y = v.y / f;
    return result;
}

Vec2_F32& operator/=(Vec2_F32& v, F32 f) {
    v.x /= f;
    v.y /= f;
    return v;
}

B32 operator==(Vec2_F32 v1, Vec2_F32 v2) {
    B32 result = ((v1.x == v2.x) && (v1.y == v2.y)); 
    return result;
}

B32 operator!=(Vec2_F32 v1, Vec2_F32 v2) {
    B32 result = !(v1 == v2); 
    return result;
}

Vec2_S32 operator+(Vec2_S32 v1, Vec2_S32 v2) {
    Vec2_S32 result = {};
    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    return result;
}

Vec2_S32& operator+=(Vec2_S32& v1, Vec2_S32 v2) {
    v1.x += v2.x;
    v1.y += v2.y;
    return v1;
}

Vec2_S32 operator-(Vec2_S32 v1, Vec2_S32 v2) {
    Vec2_S32 result = {};
    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    return result;
}

Vec2_S32& operator-=(Vec2_S32& v1, Vec2_S32 v2) {
    v1.x -= v2.x;
    v1.y -= v2.y;
    return v1;
}

Vec2_S32 operator*(Vec2_S32 v1, F32 f) {
    Vec2_S32 result = {};
    result.x = (S32)(v1.x * f);
    result.y = (S32)(v1.y * f);
    return result;
}

Vec2_S32 operator*(F32 f, Vec2_S32 v1) {
    Vec2_S32 result = {};
    result.x = (S32)(v1.x * f);
    result.y = (S32)(v1.y * f);
    return result;
}

B32 operator==(Vec2_S32 v1, Vec2_S32 v2) {
    B32 result = ((v1.x == v2.x) && (v1.y == v2.y)); 
    return result;
}

B32 operator!=(Vec2_S32 v1, Vec2_S32 v2) {
    B32 result = !(v1 == v2); 
    return result;
}

F32 Vec2_f32_dot(Vec2_F32 v1, Vec2_F32 v2) {
    F32 result = (v1.x * v2.x) + (v1.y + v2.y);
    return result;
}

Vec2_F32 vec2_f32_from_vec2_s32(Vec2_S32 v) {
    Vec2_F32 result = {};
    result.x = (S32)v.x;
    result.y = (S32)v.y;
    return result;
}
// ================================================================

// == Some regular once
S32 floor_F32_to_S32(F32 f) {
    S32 result = (S32)floorf(f);
    return result;
}
// ================================================================

// Example: 0b100100 -> returns true, idx -> 2
B32 find_idx_of_first_least_significant_set_bit(U32 value, U32* idx) {
    // TODO: implement using a macro + a compiler switch + a compiler specific instrisic

// #if COMPILER_MSVC
//     B32 found = _BitScanForward((unsigned long*)idx, mask);
//     return found;
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
}

// ============================================














