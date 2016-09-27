#ifndef CORE_PATH_H
#define CORE_PATH_H

const char PATH_USE_OS_SEPARATOR = 0;

void PathGetProgramDirectory(char* dst, unsigned dstChars);

// 'sep' specifies the directory separator to use. This should be either a
// forward-slash or a backslash.
// A value of PATH_USE_OS_SEPARATOR can also be specified for 'sep', in which
// case a backslash will be used on Windows, and a forward-slash on other
// operating systems.
bool PathAppendPath(char* dst, unsigned dstChars, const char* src, char sep);

void PathReplaceExtension(char* path, unsigned chars, const char* src,
                          const char* newExtension);

#endif // CORE_PATH_H
