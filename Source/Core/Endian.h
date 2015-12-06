#ifndef ENDIAN_H
#define ENDIAN_H

#include "Core/Types.h"

inline u16 EndianSwap16(u16 n)
{
    return (n << 8) | (n >> 8);
}

inline u32 EndianSwap32(u32 n)
{
    return ((n & 0xFF000000) >> 24) |
           ((n & 0x00FF0000) >> 8) |
           ((n & 0x0000FF00) << 8) |
           ((n & 0x000000FF) << 24);
}

union EndianU32F32 {
    u32 asU32;
    f32 asF32;
};

inline f32 EndianSwapFloat32(f32 n)
{
    EndianU32F32 u;
    u.asF32 = n;
    u.asU32 = EndianSwap32(u.asU32);
    return u.asF32;
}

inline u16 EndianSwapLE16(u16 n)
{
#ifdef ENDIAN_BIG
    return EndianSwap16(n);
#else
    return n;
#endif
}

inline u32 EndianSwapLE32(u32 n)
{
#ifdef ENDIAN_BIG
    return EndianSwap32(n);
#else
    return n;
#endif
}

inline f32 EndianSwapLEFloat32(f32 n)
{
#ifdef ENDIAN_BIG
    return EndianSwapF32(n);
#else
    return n;
#endif
}

#endif // ENDIAN_H
