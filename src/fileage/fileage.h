#ifndef SRC_FILEAGE_FILEAGE_H
#define SRC_FILEAGE_FILEAGE_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <chrono>

// Structure to represent a file with age information
struct AgedFileEntry {
    std::wstring path;
    uint64_t size;
    std::chrono::system_clock::time_point creation_time;
    std::chrono::system_clock::time_point last_write_time;
    std::chrono::system_clock::time_point last_access_time;
    std::chrono::system_clock::time_point change_time;
    
    // Age calculations
    std::chrono::milliseconds age_since_creation;
    std::chrono::milliseconds age_since_last_write;
    std::chrono::milliseconds age_since_last_access;
    
    AgedFileEntry() 
        : size(0), 
          age_since_creation(0),
          age_since_last_write(0),
          age_since_last_access(0) {}
};

// File age analysis options
struct FileAgeOptions {
    bool include_creation_time = true;
    bool include_last_write_time = true;
    bool include_last_access_time = true;
    bool include_change_time = true;
    
    // Age thresholds
    std::chrono::hours min_age_hours = std::chrono::hours(24 * 30); // 30 days
    std::chrono::hours max_age_hours = std::chrono::hours(24 * 365 * 10); // 10 years
    
    // Sorting options
    enum class SortBy {
        CREATION_TIME,
        LAST_WRITE_TIME,
        LAST_ACCESS_TIME,
        SIZE,
        PATH
    };
    
    SortBy sort_by = SortBy::LAST_ACCESS_TIME;
    bool sort_descending = true;
    
    // Filtering options
    std::vector<std::wstring> include_extensions; // Empty = all extensions
    std::vector<std::wstring> exclude_extensions; // Extensions to exclude
    std::vector<std::wstring> include_paths;     // Paths to include
    std::vector<std::wstring> exclude_paths;     // Paths to exclude
    
    // Size filtering
    uint64_t min_file_size = 0; // 0 = no minimum
    uint64_t max_file_size = 0; // 0 = no maximum
};

// File age analyzer
class FileAgeAnalyzer {
public:
    FileAgeAnalyzer();
    ~FileAgeAnalyzer();
    
    // Analyze file ages in a directory
    std::vector<AgedFileEntry> analyze_ages(const std::wstring& path, 
                                          const FileAgeOptions& options);
    
    // Get old files (older than threshold)
    std::vector<AgedFileEntry> get_old_files(const std::vector<AgedFileEntry>& files,
                                           std::chrono::hours threshold);
    
    // Get recently accessed files
    std::vector<AgedFileEntry> get_recent_files(const std::vector<AgedFileEntry>& files,
                                              std::chrono::hours threshold);
    
    // Sort files by age
    void sort_files(std::vector<AgedFileEntry>& files, 
                   FileAgeOptions::SortBy sort_by,
                   bool descending = true);
    
    // Export to CSV
    bool export_to_csv(const std::vector<AgedFileEntry>& files,
                      const std::wstring& output_path);
    
    // Export to JSON
    bool export_to_json(const std::vector<AgedFileEntry>& files,
                       const std::wstring& output_path);
    
    // Statistics
    struct AgeStatistics {
        uint64_t total_files;
        uint64_t total_size;
        std::chrono::system_clock::time_point oldest_file_date;
        std::chrono::system_clock::time_point newest_file_date;
        std::chrono::hours average_age_hours;
        std::chrono::hours median_age_hours;
        std::chrono::hours p95_age_hours;
        std::chrono::hours p99_age_hours;
        
        AgeStatistics() 
            : total_files(0), total_size(0), average_age_hours(0),
              median_age_hours(0), p95_age_hours(0), p99_age_hours(0) {}
    };
    
    AgeStatistics calculate_statistics(const std::vector<AgedFileEntry>& files);
    
private:
    // Platform-specific file time conversion
    std::chrono::system_clock::time_point file_time_to_system_time(uint64_t file_time) const;
    
    // Check if file matches filters
    bool matches_filters(const AgedFileEntry& entry, const FileAgeOptions& options) const;
    
    // Get file extension
    std::wstring get_file_extension(const std::wstring& path) const;
    
    // Recursive directory traversal
    void traverse_directory(const std::wstring& path,
                          const FileAgeOptions& options,
                          std::vector<AgedFileEntry>& results);
    
    // Process individual file
    bool process_file(const std::wstring& path, AgedFileEntry& entry);
    
    // Calculate age statistics
    std::vector<std::chrono::hours> get_age_distribution(const std::vector<AgedFileEntry>& files) const;
};

#endif // SRC_FILEAGE_FILEAGE_H