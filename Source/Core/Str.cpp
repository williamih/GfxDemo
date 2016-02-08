#include "Core/Str.h"
#include <string.h>

void StrCopy(char* dst, size_t dstChars, const char* src)
{
    if (!dstChars)
        return;

    while (--dstChars) {
        if ((*dst = *src) == 0)
            return;
        ++dst;
        ++src;
    }

    *dst = 0;
}

size_t StrCopyLen(char* dst, size_t dstChars, const char* src)
{
    if (!dstChars)
        return 0;

    char* dstBase = dst;
    while (--dstChars) {
        if ((*dst = *src) == 0)
            return (size_t)(dst - dstBase);
        ++dst;
        ++src;
    }

    *dst = 0;
    return (size_t)(dst - dstBase);
}

size_t StrLen(const char* str)
{
    return strlen(str);
}
