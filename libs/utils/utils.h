#ifndef LIBS_UTILS_UTILS_H
#define LIBS_UTILS_UTILS_H

#include <string>
#include <vector>
#include <cstdint>

// Cross-platform file handle wrapper
#ifdef _WIN32
#include <windows.h>
typedef HANDLE file_handle_t;
const file_handle_t INVALID_FILE_HANDLE = INVALID_HANDLE_VALUE;
#else
typedef int file_handle_t;
const file_handle_t INVALID_FILE_HANDLE = -1;
#endif

// Cross-platform path separator
#ifdef _WIN32
const char PATH_SEPARATOR = '\\';
const char PATH_SEPARATOR_STR[] = "\\";
#else
const char PATH_SEPARATOR = '/';
const char PATH_SEPARATOR_STR[] = "/";
#endif

// Cross-platform file information structure
struct file_info_t {
    std::string name;
    uint64_t size;
    bool is_directory;
    uint64_t creation_time;
    uint64_t last_modified_time;
    uint64_t last_access_time;
    
#ifdef _WIN32
    uint32_t attributes;
#else
    uint32_t permissions;
#endif
};

// Cross-platform file operations
class FileUtils {
public:
    // Open a file
    static file_handle_t open_file(const std::string& path, bool read_only = true);
    
    // Close a file
    static void close_file(file_handle_t handle);
    
    // Check if file handle is valid
    static bool is_valid_handle(file_handle_t handle);
    
    // Get file size
    static uint64_t get_file_size(file_handle_t handle);
    
    // Read file data
    static bool read_file_data(file_handle_t handle, void* buffer, size_t size, uint64_t offset = 0);
    
    // Get file information
    static bool get_file_info(const std::string& path, file_info_t& info);
    
    // List directory contents
    static std::vector<file_info_t> list_directory(const std::string& path);
    
    // Get path separator
    static char get_path_separator();
    
    // Join paths
    static std::string join_paths(const std::string& path1, const std::string& path2);
    
    // Get file extension
    static std::string get_file_extension(const std::string& path);
    
    // Convert to platform-specific path
    static std::string to_platform_path(const std::string& path);

    // Create a directory
    static bool create_directory(const std::string& path);

    // Create a memory-mapped file
    static file_handle_t create_mapped_file(const std::string& path, size_t size);

    // Map a view of a file
    static void* map_view_of_file(file_handle_t handle, size_t size);

    // Unmap a view of a file
    static void unmap_view_of_file(void* view, size_t size);
};

// Cross-platform string utilities
class StringUtils {
public:
    // Convert UTF-8 to wide string (Windows) or keep as-is (Linux)
    static std::wstring to_wide_string(const std::string& str);
    
    // Convert wide string to UTF-8 (Windows) or keep as-is (Linux)
    static std::string to_utf8_string(const std::wstring& wstr);
    
    // Case-insensitive string comparison
    static bool iequals(const std::string& a, const std::string& b);
    
    // Convert to lowercase
    static std::string to_lower(const std::string& str);
    
    // Split string by delimiter
    static std::vector<std::string> split(const std::string& str, char delimiter);
};

// Cross-platform time utilities
class TimeUtils {
public:
    // Get current time in milliseconds since epoch
    static uint64_t get_current_time_ms();
    
    // Convert file time to milliseconds
    static uint64_t file_time_to_ms(uint64_t file_time);
    
    // Get time difference in milliseconds
    static uint64_t time_diff_ms(uint64_t start, uint64_t end);
};

// Cross-platform system utilities
class SystemUtils {
public:
    // Get number of CPU cores
    static unsigned int get_cpu_cores();
    
    // Get available memory in bytes
    static uint64_t get_available_memory();
    
    // Get total memory in bytes
    static uint64_t get_total_memory();
    
    // Get page size
    static size_t get_page_size();
    
    // Check if running with elevated privileges
    static bool is_elevated();
};

#endif // LIBS_UTILS_UTILS_H