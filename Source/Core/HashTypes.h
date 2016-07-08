#ifndef CORE_HASHTYPES_H
#define CORE_HASHTYPES_H

#include "Core/Types.h"
#include "Core/Str.h"
#include <stddef.h>

struct HashKey_Str {
    HashKey_Str() : str(NULL) {}
    HashKey_Str(const char* s) : str(s) {}

    u32 GetHashValue() const;

    const char* str;
};

inline bool operator==(const HashKey_Str& a, const HashKey_Str& b)
{
    return StrCmp(a.str, b.str) == 0;
}

#endif // CORE_HASHTYPES_H
