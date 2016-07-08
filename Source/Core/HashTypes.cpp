#include "Core/HashTypes.h"
#include <ctype.h>

namespace {
    enum CaseSensitivity {
        Case_Sensitive,
        Case_Unsensitive,
    };

    template<CaseSensitivity S>
    struct StringChar;

    template<>
    struct StringChar<Case_Sensitive> {
        static u32 Get(u32 raw) { return raw; }
    };

    template<>
    struct StringChar<Case_Unsensitive> {
        static u32 Get(u32 raw) { return toupper(raw); }
    };
}

template<CaseSensitivity S>
static u32 Hash_Murmur32(const char* str, u32 seed)
{
    const u32 m = 0x5bd1e995;
    const u32 r = 24;

    u32 len = (u32)StrLen(str);
    u32 h = seed ^ len;

    const u8* data = (const u8*)str;

    while (len >= 4) {
        u32 k;

        k  = StringChar<S>::Get(data[0]);
        k |= StringChar<S>::Get(data[1]) << 8;
        k |= StringChar<S>::Get(data[2]) << 16;
        k |= StringChar<S>::Get(data[3]) << 24;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    switch (len) {
        case 3: h ^= StringChar<S>::Get(data[2]) << 16;
        case 2: h ^= StringChar<S>::Get(data[1]) << 8;
        case 1: h ^= StringChar<S>::Get(data[0]);
        h *= m;
    }

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

u32 HashKey_Str::GetHashValue() const
{
    const u32 seed = 0;
    return Hash_Murmur32<Case_Sensitive>(str, seed);
}
