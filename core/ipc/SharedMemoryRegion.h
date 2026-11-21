#pragma once

#include <cstddef>
#include <string>

namespace mixwave::ipc
{
    // Thin RAII wrapper around a Windows named file mapping (shared memory).
    class SharedMemoryRegion
    {
    public:
        SharedMemoryRegion() = default;
        ~SharedMemoryRegion() { close(); }

        SharedMemoryRegion (const SharedMemoryRegion&) = delete;
        SharedMemoryRegion& operator= (const SharedMemoryRegion&) = delete;

        // Opens the mapping if it exists, otherwise creates it (zero-filled).
        bool openOrCreate (const std::wstring& name, std::size_t sizeBytes);
        void close();

        void* data() const noexcept         { return view; }
        std::size_t size() const noexcept   { return mappedSize; }
        bool isOpen() const noexcept        { return view != nullptr; }

    private:
        void* mappingHandle = nullptr;
        void* view = nullptr;
        std::size_t mappedSize = 0;
    };
}
