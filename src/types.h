#pragma once

#include <stdint.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef s32 b32;

typedef size_t memory_index;

#define Pi32 3.14159265359f
#define PiOverTwo32 1.57079632679f
#define Tau32 6.28318530717958647692f
#define RadiansPerDegree (Pi32 / 180.0f)
#define U32_MAX ~0u

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define local_access static
#define internal_access static
#define class_access static

#ifdef NOT_DEBUG
#define Assert(Expression)
#else
#define Assert(Expression) if(!(Expression)) {*(u32 *)0 = 0;}
#endif

#define InvalidCodePath Assert(!"InvalidCodePath");

#define MIN(x, y) ((x < y) ? x : y)
#define MAX(x, y) ((x > y) ? x : y)
#define CLAMP(c, lo, hi) MAX(lo, MIN(hi, c))