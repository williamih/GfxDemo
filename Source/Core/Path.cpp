#include "Core/Path.h"

#if defined(_WIN32)
#  include <Windows.h>
#  include "Shlwapi.h"
#  pragma comment(lib, "Shlwapi.lib")
#elif defined(__APPLE__)
#  include <CoreFoundation/CoreFoundation.h>
#else
#  error Path module not implemented for this OS
#endif

#include "Core/Macros.h"
#include "Core/Str.h"

void PathGetProgramDirectory(char* dst, unsigned dstChars)
{
#if defined(_WIN32)
    // TODO: Use the wchar versions of these functions.
    unsigned result = GetModuleFileNameA(NULL, dst, dstChars);
    if (result >= dstChars)
        dst[0] = 0;
    char* filename = PathFindFileNameA(dst);
    if (filename != dst)
        filename[0] = 0;
#elif defined(__APPLE__)
    CFURLRef url1 = CFBundleCopyBundleURL(CFBundleGetMainBundle());
    CFURLRef url2 = CFURLCreateCopyDeletingLastPathComponent(NULL, url1);
    CFRelease(url1);
    if (!CFURLGetFileSystemRepresentation(url2, true, (UInt8*)dst, dstChars))
        FATAL("CFURLGetFileSystemRepresentation");
    CFRelease(url2);
#endif
}

bool PathAppendPath(char* dst, unsigned dstChars, const char* src, char sep)
{
    size_t len = StrLen(dst);
    char* p = dst + len - 1;
    for (; p != dst && (*p == '/' || *p == '\\'); --p)
        ;
    if (p + 1 == dst + dstChars)
        return false;
    if (sep == PATH_USE_OS_SEPARATOR) {
#ifdef _WIN32
        sep = '\\';
#else
        sep = '/';
#endif
    }
    ++p;
    *p++ = sep;
    size_t res = StrCopyLen(p, (size_t)(dst + dstChars - p), src);
    return src[res] == 0;
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
