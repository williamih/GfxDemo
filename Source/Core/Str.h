#ifndef CORE_STR_H
#define CORE_STR_H

#include <stdio.h>

void StrCopy(char* dst, size_t dstChars, const char* src);
size_t StrCopyLen(char* dst, size_t dstChars, const char* src);

size_t StrLen(const char* str);

#endif // CORE_STR_H

