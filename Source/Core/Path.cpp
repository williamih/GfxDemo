#include "Core/Path.h"
#include "Core/Macros.h"
#include "Core/Str.h"

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
