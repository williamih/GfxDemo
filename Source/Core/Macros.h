#ifndef MACROS_H
#define MACROS_H

#include "Core/Log.h"

#define ASSERT(test) FatalAssert(#test, __FILE__, __LINE__)

#define STATIC_ASSERT(expr, message) static_assert(expr, message)

#define FATAL(format, ...) Fatal(format, __FILE__, __LINE__, __VA_ARGS__)

#endif // MACROS_H
