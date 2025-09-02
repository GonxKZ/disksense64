#ifndef PLATFORM_IO_FILE_STREAM_H
#define PLATFORM_IO_FILE_STREAM_H

#include <windows.h>
#include <cstdint>
#include <memory>
#include <vector>

// Forward declarations
class IoCompletionPort;

// Buffer alignment helper
class BufferAllocator {
public:
    static constexpr size_t SECTOR_SIZE = 512;
    static constexpr size_t DEFAULT_ALIGNMENT = 4096;
    
    // Allocate aligned buffer
    static void* allocate_aligned(size_t size, size_t alignment = DEFAULT_ALIGNMENT);
    
    // Free aligned buffer
    static void free_aligned(void* ptr);
    
    // Align size to sector boundary
    static size_t align_to_sector(size_t size);
};

// File stream base class
class FileStream {
protected:
    HANDLE m_handle;
    bool m_is_valid;
    uint64_t m_file_size;
    
public:
    FileStream() : m_handle(INVALID_HANDLE_VALUE), m_is_valid(false), m_file_size(0) {}
    virtual ~FileStream();
    
    // Open file
    virtual bool open(const wchar_t* path, bool read_only = true) = 0;
    
    // Close file
    virtual void close();
    
    // Check if stream is valid
    bool is_valid() const { return m_is_valid; }
    
    // Get file size
    uint64_t get_file_size() const { return m_file_size; }
    
    // Read data asynchronously
    virtual bool read_async(uint64_t offset, void* buffer, size_t size, 
                           OVERLAPPED* overlapped) = 0;
    
    // Wait for async read completion
    virtual bool wait_read_completion(OVERLAPPED* overlapped, DWORD* bytes_read) = 0;
};

// Buffered stream (uses system cache)
class BufferedStream : public FileStream {
private:
    bool m_sequential_scan;
    
public:
    BufferedStream(bool sequential_scan = true);
    ~BufferedStream() override;
    
    bool open(const wchar_t* path, bool read_only = true) override;
    bool read_async(uint64_t offset, void* buffer, size_t size, 
                   OVERLAPPED* overlapped) override;
    bool wait_read_completion(OVERLAPPED* overlapped, DWORD* bytes_read) override;
};

// Raw stream (no buffering, requires alignment)
class RawStream : public FileStream {
private:
    size_t m_sector_size;
    size_t m_alignment;
    
public:
    RawStream();
    ~RawStream() override;
    
    bool open(const wchar_t* path, bool read_only = true) override;
    bool read_async(uint64_t offset, void* buffer, size_t size, 
                   OVERLAPPED* overlapped) override;
    bool wait_read_completion(OVERLAPPED* overlapped, DWORD* bytes_read) override;
    
    // Get required alignment
    size_t get_alignment() const { return m_alignment; }
    
    // Align offset to sector boundary
    uint64_t align_offset(uint64_t offset) const;
    
    // Align size to sector boundary
    size_t align_size(size_t size) const;
};

// Stream factory
class StreamFactory {
public:
    enum StreamType {
        BUFFERED_STREAM,
        RAW_STREAM
    };
    
    // Create appropriate stream based on requirements
    static std::unique_ptr<FileStream> create_stream(StreamType type);
    
    // Auto-select stream based on file size and policy
    static std::unique_ptr<FileStream> create_optimal_stream(
        const wchar_t* path, 
        uint64_t file_size,
        bool no_buffering_policy = false);
};

#endif // PLATFORM_IO_FILE_STREAM_H