#include "Core/Str.h"
#include <string.h>
#include <stdio.h> // for vsnprintf

int StrPrintf(char* dst, size_t dstChars, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int result = StrPrintfV(dst, dstChars, format, args);
    va_end(args);
    return result;
}

int StrPrintfV(char* dst, size_t dstChars, const char* format, va_list args)
{
    return vsnprintf(dst, dstChars, format, args);
}

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

int StrCmp(const char* str1, const char* str2)
{
    return strcmp(str1, str2);
}
