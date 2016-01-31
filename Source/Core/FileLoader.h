#ifndef CORE_FILELOADER_H
#define CORE_FILELOADER_H

#include "Core/Types.h"

class FileLoader {
public:
    void Load(const char* path, u8** data, u32* size,
              void* (*allocate)(u32 size, void* userdata), void* userdata);
};

#endif // CORE_FILELOADER_H
