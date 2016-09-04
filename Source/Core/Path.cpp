#include "Core/Path.h"
#include "Core/Macros.h"
#include "Core/Str.h"

const char* PathFindExtension(const char* path)
{
    size_t len = StrLen(path);
    const char* p = path + len;
    for (; p != path && *p != '.'; --p)
        ;
    if (p == path)
        return NULL;
    return p;
}

void PathReplaceExtension(char* path, unsigned chars, const char* src,
                          const char* newExtension)
{
    ASSERT(newExtension[0] == '.');
    size_t len;
    if (path == src)
        len = StrLen(path);
    else
        len = StrCopyLen(path, chars, src);
    char* p = path + len;
    for ( ; *p != '.' && p != path; --p)
        ;
    if (*p != '.')
        return;
    StrCopy(p, chars - (p - path), newExtension);
}
