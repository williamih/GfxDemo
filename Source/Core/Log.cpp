#include "Core/Log.h"

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <stdlib.h> // for abort()
#endif

#include <stdio.h>

#include "Core/Str.h"

void FatalError()
{
#ifdef _WIN32
    DebugBreak();
#else
    abort();
#endif
}

void Fatal(const char* format, const char* file, int line, ...)
{
    char message[256];
    va_list args;
    va_start(args, line);
    StrPrintfV(message, sizeof message, format, args);
    va_end(args);

    LogError("Fatal error!");
    LogError("\tFile: %s", file);
    LogError("\tLine: %d", line);
    LogError("\tMessage: %s", message);

    FatalError();
}

void FatalAssert(const char* message, const char* file, int line)
{
    LogError("Assertion failed!");
    LogError("\tFile: %s", file);
    LogError("\tLine: %d", line);
    LogError("\tExpr: %s", message);

    FatalError();
}

void LogErrorV(const char* format, va_list args)
{
#ifdef _WIN32
    char buffer[1024];
    int charsWritten = vsnprintf(buffer, sizeof buffer - 1, format, args);
    if (charsWritten >= sizeof buffer)
        charsWritten = sizeof buffer - 1;
    if (charsWritten + 1 < sizeof buffer) {
        buffer[charsWritten] = '\n';
        buffer[charsWritten + 1] = '\0';
    } else {
        buffer[sizeof buffer - 1] = '\0';
    }
    OutputDebugStringA(buffer);
#else
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
#endif
}

void LogError(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LogErrorV(format, args);
    va_end(args);
}
