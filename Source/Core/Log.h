#ifndef CORE_LOG_H
#define CORE_LOG_H

#include <stdarg.h>

void FatalError();
void Fatal(const char* format, const char* file, int line, ...);
void FatalAssert(const char* message, const char* file, int line);

void LogErrorV(const char* format, va_list args);
void LogError(const char* format, ...);

#endif // CORE_LOG_H
