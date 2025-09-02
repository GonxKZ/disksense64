#include "file_stream.h"
#include <iostream>

// BufferAllocator implementation
void* BufferAllocator::allocate_aligned(size_t size, size_t alignment) {
    return _aligned_malloc(size, alignment);
}

void BufferAllocator::free_aligned(void* ptr) {
    _aligned_free(ptr);
}

size_t BufferAllocator::align_to_sector(size_t size) {
    return (size + SECTOR_SIZE - 1) & ~(SECTOR_SIZE - 1);
}

// FileStream implementation
FileStream::~FileStream() {
    close();
}

void FileStream::close() {
    if (m_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
    m_is_valid = false;
    m_file_size = 0;
}

// BufferedStream implementation
BufferedStream::BufferedStream(bool sequential_scan) 
    : m_sequential_scan(sequential_scan) {
}

BufferedStream::~BufferedStream() {
}

bool BufferedStream::open(const wchar_t* path, bool read_only) {
    DWORD access = read_only ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE);
    DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    DWORD creation = read_only ? OPEN_EXISTING : OPEN_ALWAYS;
    DWORD flags = FILE_ATTRIBUTE_NORMAL;
    
    if (m_sequential_scan) {
        flags |= FILE_FLAG_SEQUENTIAL_SCAN;
    }
    
    m_handle = CreateFileW(
        path,
        access,
        share_mode,
        nullptr,
        creation,
        flags,
        nullptr
    );
    
    if (m_handle == INVALID_HANDLE_VALUE) {
        m_is_valid = false;
        return false;
    }
    
    // Get file size
    LARGE_INTEGER size;
    if (GetFileSizeEx(m_handle, &size)) {
        m_file_size = static_cast<uint64_t>(size.QuadPart);
    } else {
        m_file_size = 0;
    }
    
    m_is_valid = true;
    return true;
}

bool BufferedStream::read_async(uint64_t offset, void* buffer, size_t size, 
                               OVERLAPPED* overlapped) {
    if (!m_is_valid || !overlapped || !buffer) {
        return false;
    }
    
    // Set offset in OVERLAPPED structure
    overlapped->Offset = static_cast<DWORD>(offset & 0xFFFFFFFF);
    overlapped->OffsetHigh = static_cast<DWORD>((offset >> 32) & 0xFFFFFFFF);
    
    // Perform async read
    return ReadFile(m_handle, buffer, static_cast<DWORD>(size), nullptr, overlapped) || 
           GetLastError() == ERROR_IO_PENDING;
}

bool BufferedStream::wait_read_completion(OVERLAPPED* overlapped, DWORD* bytes_read) {
    if (!m_is_valid || !overlapped || !bytes_read) {
        return false;
    }
    
    return GetOverlappedResult(m_handle, overlapped, bytes_read, TRUE);
}

// RawStream implementation
RawStream::RawStream() 
    : m_sector_size(SECTOR_SIZE), m_alignment(DEFAULT_ALIGNMENT) {
}

RawStream::~RawStream() {
}

bool RawStream::open(const wchar_t* path, bool read_only) {
    DWORD access = read_only ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE);
    DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    DWORD creation = read_only ? OPEN_EXISTING : OPEN_ALWAYS;
    DWORD flags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED;
    
    m_handle = CreateFileW(
        path,
        access,
        share_mode,
        nullptr,
        creation,
        flags,
        nullptr
    );
    
    if (m_handle == INVALID_HANDLE_VALUE) {
        m_is_valid = false;
        return false;
    }
    
    // Get sector size from volume
    DWORD bytes_returned;
    FILE_FS_SECTOR_SIZE_INFORMATION sector_info;
    if (DeviceIoControl(m_handle, FSCTL_QUERY_SECTOR_SIZE_INFO, nullptr, 0,
                        &sector_info, sizeof(sector_info), &bytes_returned, nullptr)) {
        m_sector_size = sector_info.PhysicalSectorSize;
        m_alignment = sector_info.PhysicalSectorSize;
    }
    
    // Get file size
    LARGE_INTEGER size;
    if (GetFileSizeEx(m_handle, &size)) {
        m_file_size = static_cast<uint64_t>(size.QuadPart);
    } else {
        m_file_size = 0;
    }
    
    m_is_valid = true;
    return true;
}

bool RawStream::read_async(uint64_t offset, void* buffer, size_t size, 
                          OVERLAPPED* overlapped) {
    if (!m_is_valid || !overlapped || !buffer) {
        return false;
    }
    
    // Align offset and size to sector boundaries
    uint64_t aligned_offset = align_offset(offset);
    size_t aligned_size = align_size(size);
    
    // Set offset in OVERLAPPED structure
    overlapped->Offset = static_cast<DWORD>(aligned_offset & 0xFFFFFFFF);
    overlapped->OffsetHigh = static_cast<DWORD>((aligned_offset >> 32) & 0xFFFFFFFF);
    
    // Perform async read
    return ReadFile(m_handle, buffer, static_cast<DWORD>(aligned_size), nullptr, overlapped) || 
           GetLastError() == ERROR_IO_PENDING;
}

bool RawStream::wait_read_completion(OVERLAPPED* overlapped, DWORD* bytes_read) {
    if (!m_is_valid || !overlapped || !bytes_read) {
        return false;
    }
    
    return GetOverlappedResult(m_handle, overlapped, bytes_read, TRUE);
}

uint64_t RawStream::align_offset(uint64_t offset) const {
    return offset & ~(static_cast<uint64_t>(m_alignment) - 1);
}

size_t RawStream::align_size(size_t size) const {
    return (size + m_alignment - 1) & ~(m_alignment - 1);
}

// StreamFactory implementation
std::unique_ptr<FileStream> StreamFactory::create_stream(StreamType type) {
    switch (type) {
        case BUFFERED_STREAM:
            return std::make_unique<BufferedStream>();
        case RAW_STREAM:
            return std::make_unique<RawStream>();
        default:
            return nullptr;
    }
}

std::unique_ptr<FileStream> StreamFactory::create_optimal_stream(
    const wchar_t* path, 
    uint64_t file_size,
    bool no_buffering_policy) {
    
    // If no buffering policy is enabled and file is large enough, use raw stream
    if (no_buffering_policy && file_size > 1024 * 1024) { // 1MB threshold
        return std::make_unique<RawStream>();
    }
    
    // Otherwise use buffered stream
    return std::make_unique<BufferedStream>();
}