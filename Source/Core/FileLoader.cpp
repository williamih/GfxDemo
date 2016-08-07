#include "Core/FileLoader.h"
#include <stdio.h>
#include <string.h>
#include "Core/Str.h"
#include "Core/Macros.h"

FileLoader::FileLoader(const char* basePath)
    : m_basePath(NULL)
    , m_basePathLen(0)
{
    m_basePathLen = StrLen(basePath);
    if (m_basePathLen != 0) {
        m_basePath = (char*)malloc(m_basePathLen + 1);
        memcpy(m_basePath, basePath, m_basePathLen + 1);
    }
}

FileLoader::~FileLoader()
{
    free(m_basePath);
}

void FileLoader::Load(const char* path, u8** data, u32* size,
                      void* (*allocate)(u32 size, void* userdata), void* userdata)
{
    // Work out the full path
    size_t pathLen = StrLen(path);
    size_t fullPathLen = m_basePathLen + pathLen + 1;
    if (m_basePathLen != 1)
        ++fullPathLen; // for the directory separator character

    char* fullPath = (char*)malloc(fullPathLen);
    if (m_basePathLen == 0) {
        memcpy(fullPath, path, pathLen + 1);
    } else {
        memcpy(fullPath, m_basePath, m_basePathLen);
        fullPath[m_basePathLen] = '\\';
        memcpy(fullPath + m_basePathLen + 1, path, pathLen + 1);
    }

#ifndef _WIN32
    // Convert slashes
    for (char* p = fullPath; *p; ++p) {
        if (*p == '\\')
            *p = '/';
    }
#endif

    // Actually do the I/O
    FILE* file;
    long length;
    u8* bytes = NULL;
    bool result = false;

    for (;;) {
        if (!(file = fopen(fullPath, "rb")))
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

    // Clean up
    free(fullPath);
    if (file) fclose(file);
    if (!result)
        FATAL("Failed to read file %s", path);
}
