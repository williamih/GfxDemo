#include "Core/File.h"
#include <stdio.h>
#include "Core/Macros.h"

void FileReadFile(const char* path, u8** data, u32* size,
                  void* (*allocate)(u32 size, void* userdata), void* userdata)
{
    FILE* file;
    long length;
    u8* bytes = NULL;
    bool result = false;

    for (;;) {
        if (!(file = fopen(path, "rb")))
            break;

        if (fseek(file, 0, SEEK_END))
            break;

        if ((length = ftell(file)) == -1)
            break;

        if (fseek(file, 0, SEEK_SET))
            break;

        bytes = (u8*)allocate((u32)length, userdata);

        if (fread(&bytes[0], 1, (size_t)length, file) != (size_t)length)
            break;

        if (data) *data = bytes;
        if (size) *size = (u32)length;
        bytes = NULL;
        result = true;
        break;
    }

    if (file) fclose(file);
    if (!result)
        FATAL("Failed to read file %s", path);
}
