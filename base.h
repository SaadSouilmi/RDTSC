#ifndef BASE_H
#define BASE_H

#include <stdint.h>

#define PI 3.14159265358979323846


typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int8_t  I8;
typedef int16_t I16;
typedef int32_t I32;
typedef int64_t I64;

typedef float  F32;
typedef double F64;
typedef double _Complex complex64;


#define Min(a, b) (((a) < (b)) ? (a) : (b))
#define Max(a, b) (((a) > (b)) ? (a) : (b))
#define Abs(a)    (((a) < 0)   ? -(a) : (a))

#define internal static
#define global static
#define local_persist static
#define read_only static const

#endif



