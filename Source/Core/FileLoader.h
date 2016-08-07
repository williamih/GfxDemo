#ifndef CORE_FILELOADER_H
#define CORE_FILELOADER_H

#include <stddef.h>
#include "Core/Types.h"

class FileLoader {
public:
    explicit FileLoader(const char* basePath = NULL);
    ~FileLoader();

    void Load(const char* path, u8** data, u32* size,
              void* (*allocate)(u32 size, void* userdata), void* userdata);

private:
    char* m_basePath;
    size_t m_basePathLen;
};

#endif // CORE_FILELOADER_H
