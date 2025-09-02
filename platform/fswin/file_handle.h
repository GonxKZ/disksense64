#ifndef PLATFORM_FSWIN_FILE_HANDLE_H
#define PLATFORM_FSWIN_FILE_HANDLE_H

#include <windows.h>

// RAII wrapper for Windows file handles
class FileHandle {
private:
    HANDLE m_handle;

public:
    // Constructor
    explicit FileHandle(HANDLE handle = INVALID_HANDLE_VALUE) : m_handle(handle) {}
    
    // Destructor - automatically closes the handle
    ~FileHandle() {
        if (isValid()) {
            CloseHandle(m_handle);
        }
    }
    
    // Move constructor
    FileHandle(FileHandle&& other) noexcept : m_handle(other.m_handle) {
        other.m_handle = INVALID_HANDLE_VALUE;
    }
    
    // Move assignment operator
    FileHandle& operator=(FileHandle&& other) noexcept {
        if (this != &other) {
            if (isValid()) {
                CloseHandle(m_handle);
            }
            m_handle = other.m_handle;
            other.m_handle = INVALID_HANDLE_VALUE;
        }
        return *this;
    }
    
    // Delete copy constructor and assignment operator
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
    
    // Get the raw handle
    HANDLE get() const {
        return m_handle;
    }
    
    // Check if the handle is valid
    bool isValid() const {
        return m_handle != INVALID_HANDLE_VALUE && m_handle != nullptr;
    }
    
    // Release ownership of the handle
    HANDLE release() {
        HANDLE handle = m_handle;
        m_handle = INVALID_HANDLE_VALUE;
        return handle;
    }
    
    // Close the handle explicitly
    void close() {
        if (isValid()) {
            CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
        }
    }
};

#endif // PLATFORM_FSWIN_FILE_HANDLE_H