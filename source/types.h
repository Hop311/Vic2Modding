#pragma once

#include <stdint.h>

#define ERROR_RETURN -1

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef int64_t s64;
typedef uint64_t u64;

typedef u8 boolean;
#define true 1
#define false 0
#define TO_BOOL(x) ((x)!=0)

#define internal static
#define local static

typedef union vec2_t {
	struct { s32 x, y; };
	s32 s[2];
} vec2;

typedef union vec2f_t {
	struct { float x, y; };
	float s[2];
} vec2f;
