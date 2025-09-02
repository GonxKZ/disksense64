#ifndef LIBS_UTILS_FILE_UTILS_H
#define LIBS_UTILS_FILE_UTILS_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <functional>

// Platform-independent file handle type
#ifdef _WIN32
#include <windows.h>
typedef HANDLE file_handle_t;
const file_handle_t INVALID_FILE_HANDLE = INVALID_HANDLE_VALUE;
#else
typedef int file_handle_t;
const file_handle_t INVALID_FILE_HANDLE = -1;
#endif

// File information structure
struct file_info_t {
    std::string name;
    std::string full_path;
    uint64_t size;
    bool is_directory;
    bool is_symlink;
    uint64_t creation_time;
    uint64_t last_write_time;
    uint64_t last_access_time;
    uint64_t change_time;
    
#ifdef _WIN32
    uint32_t attributes;
#else
    uint32_t permissions;
#endif
    
    file_info_t() 
        : size(0), is_directory(false), is_symlink(false),
          creation_time(0), last_write_time(0), 
          last_access_time(0), change_time(0),
#ifdef _WIN32
          attributes(0)
#else
          permissions(0)
#endif
    {}
};

// File utilities class
class FileUtils {
public:
    // Open file
    static file_handle_t open_file(const std::string& path, bool read_only = true);
    
    // Close file
    static void close_file(file_handle_t handle);
    
    // Check if file handle is valid
    static bool is_valid_handle(file_handle_t handle);
    
    // Get file size
    static uint64_t get_file_size(file_handle_t handle);
    
    // Read file data
    static bool read_file_data(file_handle_t handle, void* buffer, size_t size, uint64_t offset = 0);
    
    // Write file data
    static bool write_file_data(file_handle_t handle, const void* buffer, size_t size, uint64_t offset = 0);
    
    // Get file information
    static bool get_file_info(const std::string& path, file_info_t& info);
    
    // List directory contents
    static std::vector<file_info_t> list_directory(const std::string& path);
    
    // Create directory
    static bool create_directory(const std::string& path);
    
    // Delete file
    static bool delete_file(const std::string& path);
    
    // Move file
    static bool move_file(const std::string& source, const std::string& destination);
    
    // Copy file
    static bool copy_file(const std::string& source, const std::string& destination);
    
    // Check if path exists
    static bool path_exists(const std::string& path);
    
    // Check if path is directory
    static bool is_directory(const std::string& path);
    
    // Check if path is file
    static bool is_file(const std::string& path);
    
    // Get path separator
    static char get_path_separator();
    
    // Join paths
    static std::string join_paths(const std::string& path1, const std::string& path2);
    
    // Get file extension
    static std::string get_file_extension(const std::string& path);
    
    // Convert to platform-specific path
    static std::string to_platform_path(const std::string& path);
    
    // Get parent directory
    static std::string get_parent_directory(const std::string& path);
    
    // Get file name
    static std::string get_file_name(const std::string& path);
    
    // Get absolute path
    static std::string get_absolute_path(const std::string& path);
    
    // Normalize path
    static std::string normalize_path(const std::string& path);
    
    // Get temporary directory
    static std::string get_temp_directory();
    
    // Get home directory
    static std::string get_home_directory();
    
    // Get current working directory
    static std::string get_current_directory();
    
    // Set current working directory
    static bool set_current_directory(const std::string& path);
    
    // Get available space on volume
    static uint64_t get_available_space(const std::string& path);
    
    // Get total space on volume
    static uint64_t get_total_space(const std::string& path);
    
    // Get free space on volume
    static uint64_t get_free_space(const std::string& path);
    
    // Check if file is locked
    static bool is_file_locked(const std::string& path);
    
    // Lock file
    static bool lock_file(file_handle_t handle);
    
    // Unlock file
    static bool unlock_file(file_handle_t handle);
    
    // Flush file buffers
    static bool flush_file_buffers(file_handle_t handle);
    
    // Truncate file
    static bool truncate_file(file_handle_t handle, uint64_t size);
    
    // Get file attributes
    static uint32_t get_file_attributes(const std::string& path);
    
    // Set file attributes
    static bool set_file_attributes(const std::string& path, uint32_t attributes);
    
    // Get file times
    static bool get_file_times(const std::string& path, 
                              uint64_t& creation_time,
                              uint64_t& last_write_time,
                              uint64_t& last_access_time);
    
    // Set file times
    static bool set_file_times(const std::string& path,
                             uint64_t creation_time,
                             uint64_t last_write_time,
                             uint64_t last_access_time);
};

#endif // LIBS_UTILS_FILE_UTILS_H