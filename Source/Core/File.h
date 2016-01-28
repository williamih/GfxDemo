#ifndef CORE_FILE_H
#define CORE_FILE_H

#include "Core/Types.h"

void FileReadFile(const char* path, u8** data, u32* size,
                  void* (*allocate)(u32 size, void* userdata), void* userdata);

#endif // CORE_FILE_H
