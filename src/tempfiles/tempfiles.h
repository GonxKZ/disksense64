#ifndef SRC_TEMPFILES_TEMPFILES_H
#define SRC_TEMPFILES_TEMPFILES_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

// Structure to represent a temporary file
struct TempFileEntry {
    std::wstring path;
    uint64_t size;
    uint64_t last_access_time;
    uint64_t creation_time;
    std::wstring file_type; // cache, temp, log, etc.
    
    TempFileEntry() : size(0), last_access_time(0), creation_time(0) {}
};

// Temp file detection options
struct TempFileOptions {
    bool scan_system_temp = true;      // Scan system temp directories
    bool scan_user_temp = true;        // Scan user temp directories
    bool scan_browser_cache = true;    // Scan browser cache directories
    bool scan_application_cache = true; // Scan application cache directories
    uint64_t min_age_days = 7;         // Minimum age in days to consider for cleanup
    uint64_t max_size_mb = 1024;       // Maximum total size to scan (0 = unlimited)
    std::vector<std::wstring> exclude_patterns; // Patterns to exclude from scanning
};

// Temp files manager
class TempFileManager {
public:
    TempFileManager();
    ~TempFileManager();
    
    // Scan for temporary files
    std::vector<TempFileEntry> scan_temp_files(const TempFileOptions& options);
    
    // Estimate potential space savings
    uint64_t estimate_savings(const std::vector<TempFileEntry>& temp_files);
    
    // Clean temporary files
    bool clean_temp_files(const std::vector<TempFileEntry>& temp_files, 
                         bool simulate_only = true);
    
    // Get system temp directories
    std::vector<std::wstring> get_system_temp_directories();
    
    // Get user temp directories
    std::vector<std::wstring> get_user_temp_directories();
    
    // Get browser cache directories
    std::vector<std::wstring> get_browser_cache_directories();
    
    // Classify file type
    std::wstring classify_file_type(const std::wstring& path);
    
    // Check if file is old enough to be cleaned
    bool is_old_enough(const TempFileEntry& entry, uint64_t min_age_days);
    
private:
    // Platform-specific implementations
    std::vector<std::wstring> get_windows_temp_directories();
    std::vector<std::wstring> get_linux_temp_directories();
    
    // File classification helpers
    bool is_cache_file(const std::wstring& path);
    bool is_temp_file(const std::wstring& path);
    bool is_log_file(const std::wstring& path);
    
    // File operations
    bool delete_file_safe(const std::wstring& path);
    bool move_to_recycle_bin(const std::wstring& path);
};

#endif // SRC_TEMPFILES_TEMPFILES_H