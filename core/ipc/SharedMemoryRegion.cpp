#include "SharedMemoryRegion.h"

#ifndef NOMINMAX
 #define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
 #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdint>

namespace mixwave::ipc
{
    bool SharedMemoryRegion::openOrCreate (const std::wstring& name, std::size_t sizeBytes)
    {
        close();

        const auto size64 = static_cast<std::uint64_t> (sizeBytes);

        HANDLE h = CreateFileMappingW (INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                       static_cast<DWORD> (size64 >> 32),
                                       static_cast<DWORD> (size64 & 0xFFFFFFFFu),
                                       name.c_str());
        if (h == nullptr)
            return false;

        // If a mapping with this name already exists but is smaller than what we
        // need (older layout), this fails and we report "not connected".
        void* v = MapViewOfFile (h, FILE_MAP_ALL_ACCESS, 0, 0, sizeBytes);
        if (v == nullptr)
        {
            CloseHandle (h);
            return false;
        }

        mappingHandle = h;
        view = v;
        mappedSize = sizeBytes;
        return true;
    }

    void SharedMemoryRegion::close()
    {
        if (view != nullptr)
            UnmapViewOfFile (view);

        if (mappingHandle != nullptr)
            CloseHandle (static_cast<HANDLE> (mappingHandle));

        view = nullptr;
        mappingHandle = nullptr;
        mappedSize = 0;
    }
}
