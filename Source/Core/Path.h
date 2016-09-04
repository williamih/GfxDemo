#ifndef CORE_PATH_H
#define CORE_PATH_H

// Returns the file extension of the path (including the dot), or NULL if no
// file extension was present.
const char* PathFindExtension(const char* path);

void PathReplaceExtension(char* path, unsigned chars, const char* src,
                          const char* newExtension);

#endif // CORE_PATH_H
