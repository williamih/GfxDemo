#ifndef TEXTURE_DDSFILE_H
#define TEXTURE_DDSFILE_H

#include <vector>
#include "Core/Types.h"

enum DDSTextureFormat {
    DDSTEXFORMAT_DXT1,
    DDSTEXFORMAT_DXT3,
    DDSTEXFORMAT_DXT5,
    DDSTEXFORMAT_UNKNOWN,
};

class DDSFile {
public:
    typedef void (*PFnDestroy)(void* ptr, void* userdata);

    // The instance takes ownership of the data pointer
    DDSFile(u8* data, int size, const char* path,
            PFnDestroy destroy, void* userdata);
    ~DDSFile();

    unsigned Width() const;
    unsigned Height() const;
    unsigned MipCount() const;
    DDSTextureFormat Format() const;
    const u8* Pixels() const;

private:
    DDSFile(const DDSFile&);
    DDSFile& operator=(const DDSFile&);

    DDSTextureFormat m_texFormat;
    u8* m_data;
    PFnDestroy m_destroyFunc;
    void* m_destroyUserdata;
};

#endif // TEXTURE_DDSFILE_H
