#include "mrn_core.h"

#include <fstream>

moraine::Allocation moraine::loadFile(Logfile logfile, Stringr path)
{
    Time start = Time::now();

    std::ifstream fileStream(path.wcstr(), std::ios::binary | std::ios::ate); // Open file at end to read size

    assert(logfile, fileStream.is_open(), sprintf(L"Opening file \"%s\" failed", path.wcstr()), MRN_DEBUG_INFO); // Do error checks

    size_t fileSize = fileStream.tellg(); // Read size

    fileStream.seekg(0, std::ios::beg); // Go to beginning of file
    std::unique_ptr<uint8_t[]> fileData = std::make_unique<uint8_t[]>(fileSize); // Allocate memory
    fileStream.read(reinterpret_cast<char*>(fileData.get()), fileSize); // Read file into memory

    fileStream.close(); // close file

#ifdef _DEBUG

    float fileSizeF = static_cast<float>(fileSize);
    wchar_t sizePrefix;

    if ((fileSizeF /= 1024) < 1024.0f)
        sizePrefix = L'K';
    else if ((fileSizeF /= 1024) < 1024.0f)
        sizePrefix = L'M';
    else if ((fileSizeF /= 1024) < 1024.0f)
        sizePrefix = L'G';
    else
        sizePrefix = L'G';

    logfile->print(WHITE, sprintf(L"Loaded file \"%s\", (%.3f %cB) (%.3f ms)", path.wcstr(), fileSizeF, sizePrefix, Time::duration(start, Time::now()).getMillisecondsF()));

#endif

    return { std::move(fileData), fileSize };
}

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#pragma comment(lib, "vulkan-1.lib")