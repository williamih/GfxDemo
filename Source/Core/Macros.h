#ifndef MACROS_H
#define MACROS_H

#include <assert.h>
#include <stdio.h>

#define ASSERT(test) assert(test)

#define STATIC_ASSERT(expr, message) static_assert(expr, message)

#define FATAL(format, ...) \
    do { fprintf(stderr, format, ##__VA_ARGS__); abort(); } while (0);

#endif // MACROS_H
