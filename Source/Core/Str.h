#ifndef CORE_STR_H
#define CORE_STR_H

#include <stddef.h>
#include <stdarg.h>

int StrPrintf(char* dst, size_t dstChars, const char* format, ...);
int StrPrintfV(char* dst, size_t dstChars, const char* format, va_list args);

void StrCopy(char* dst, size_t dstChars, const char* src);
size_t StrCopyLen(char* dst, size_t dstChars, const char* src);

size_t StrLen(const char* str);
int StrCmp(const char* str1, const char* str2);

#endif // CORE_STR_H

