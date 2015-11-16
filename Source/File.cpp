#include "File.h"
#include <stdio.h>

bool FileReadFile(const char* path, std::vector<char>& data)
{
    FILE* file;
    long length;
    bool success = false;

    for (;;) {
        if (!(file = fopen(path, "rb")))
            break;

        if (fseek(file, 0, SEEK_END))
            break;

        if ((length = ftell(file)) == -1)
            break;

        if (fseek(file, 0, SEEK_SET))
            break;

        data.resize((std::vector<char>::size_type)length + 1);

        if (fread(&data[0], 1, (size_t)length, file) != (size_t)length)
            break;

        // Ensure the data is always null-terminated.
        data[length] = 0;
        success = true;
        break;
    }

    if (file) fclose(file);
    if (!success) {
        std::vector<char> empty;
        data.swap(empty);
    }
    return success;
}
