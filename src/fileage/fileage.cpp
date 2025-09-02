#include "fileage.h"
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <cmath>
#include <wrl/client.h>

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>
#endif

FileAgeAnalyzer::FileAgeAnalyzer() {
}

FileAgeAnalyzer::~FileAgeAnalyzer() {
}

std::vector<AgedFileEntry> FileAgeAnalyzer::analyze_ages(const std::wstring& path, 
                                                        const FileAgeOptions& options) {
    std::vector<AgedFileEntry> results;
    
    try {
        // Check if path exists
        if (!std::filesystem::exists(path)) {
            std::wcerr << L"Error: Path does not exist: " << path << std::endl;
            return results;
        }
        
        // Traverse directory recursively
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                AgedFileEntry file_entry;
                if (process_file(entry.path().wstring(), file_entry)) {
                    if (matches_filters(file_entry, options)) {
                        results.push_back(file_entry);
                    }
                }
            }
        }
        
        // Sort results according to options
        switch (options.sort_by) {
            case FileAgeOptions::SortBy::CREATION_TIME:
                if (options.sort_descending) {
                    std::sort(results.begin(), results.end(), 
                             [](const AgedFileEntry& a, const AgedFileEntry& b) {
                                 return a.creation_time > b.creation_time;
                             });
                } else {
                    std::sort(results.begin(), results.end(), 
                             [](const AgedFileEntry& a, const AgedFileEntry& b) {
                                 return a.creation_time < b.creation_time;
                             });
                }
                break;
                
            case FileAgeOptions::SortBy::LAST_WRITE_TIME:
                if (options.sort_descending) {
                    std::sort(results.begin(), results.end(), 
                             [](const AgedFileEntry& a, const AgedFileEntry& b) {
                                 return a.last_write_time > b.last_write_time;
                             });
                } else {
                    std::sort(results.begin(), results.end(), 
                             [](const AgedFileEntry& a, const AgedFileEntry& b) {
                                 return a.last_write_time < b.last_write_time;
                             });
                }
                break;
                
            case FileAgeOptions::SortBy::LAST_ACCESS_TIME:
                if (options.sort_descending) {
                    std::sort(results.begin(), results.end(), 
                             [](const AgedFileEntry& a, const AgedFileEntry& b) {
                                 return a.last_access_time > b.last_access_time;
                             });
                } else {
                    std::sort(results.begin(), results.end(), 
                             [](const AgedFileEntry& a, const AgedFileEntry& b) {
                                 return a.last_access_time < b.last_access_time;
                             });
                }
                break;
                
            case FileAgeOptions::SortBy::SIZE:
                if (options.sort_descending) {
                    std::sort(results.begin(), results.end(), 
                             [](const AgedFileEntry& a, const AgedFileEntry& b) {
                                 return a.size > b.size;
                             });
                } else {
                    std::sort(results.begin(), results.end(), 
                             [](const AgedFileEntry& a, const AgedFileEntry& b) {
                                 return a.size < b.size;
                             });
                }
                break;
                
            case FileAgeOptions::SortBy::PATH:
                if (options.sort_descending) {
                    std::sort(results.begin(), results.end(), 
                             [](const AgedFileEntry& a, const AgedFileEntry& b) {
                                 return a.path > b.path;
                             });
                } else {
                    std::sort(results.begin(), results.end(), 
                             [](const AgedFileEntry& a, const AgedFileEntry& b) {
                                 return a.path < b.path;
                             });
                }
                break;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error analyzing file ages: " << e.what() << std::endl;
    }
    
    return results;
}

std::vector<AgedFileEntry> FileAgeAnalyzer::get_old_files(const std::vector<AgedFileEntry>& files,
                                                         std::chrono::hours threshold) {
    std::vector<AgedFileEntry> old_files;
    auto now = std::chrono::system_clock::now();
    
    for (const auto& file : files) {
        auto age = now - file.last_write_time;
        if (age >= threshold) {
            old_files.push_back(file);
        }
    }
    
    return old_files;
}

std::vector<AgedFileEntry> FileAgeAnalyzer::get_recent_files(const std::vector<AgedFileEntry>& files,
                                                            std::chrono::hours threshold) {
    std::vector<AgedFileEntry> recent_files;
    auto now = std::chrono::system_clock::now();
    
    for (const auto& file : files) {
        auto age = now - file.last_write_time;
        if (age <= threshold) {
            recent_files.push_back(file);
        }
    }
    
    return recent_files;
}

bool FileAgeAnalyzer::export_to_csv(const std::vector<AgedFileEntry>& files,
                                   const std::wstring& output_path) {
    try {
        // Convert wide string to narrow string for ofstream
        std::string narrow_path(output_path.begin(), output_path.end());
        std::ofstream csv_file(narrow_path);
        
        if (!csv_file.is_open()) {
            return false;
        }
        
        // Write CSV header
        csv_file << "Path,Size,Creation Time,Last Write Time,Last Access Time,Age Since Creation (Hours),Age Since Last Write (Hours),Age Since Last Access (Hours)\n";
        
        // Write file data
        for (const auto& file : files) {
            // Convert path to UTF-8
            std::string path_utf8(file.path.begin(), file.path.end());
            
            // Convert times to readable format
            auto creation_time_t = std::chrono::system_clock::to_time_t(file.creation_time);
            auto last_write_time_t = std::chrono::system_clock::to_time_t(file.last_write_time);
            auto last_access_time_t = std::chrono::system_clock::to_time_t(file.last_access_time);
            
            // Format times
            std::tm creation_tm, last_write_tm, last_access_tm;
#ifdef _WIN32
            localtime_s(&creation_tm, &creation_time_t);
            localtime_s(&last_write_tm, &last_write_time_t);
            localtime_s(&last_access_tm, &last_access_time_t);
#else
            localtime_r(&creation_time_t, &creation_tm);
            localtime_r(&last_write_time_t, &last_write_tm);
            localtime_r(&last_access_time_t, &last_access_tm);
#endif
            
            char creation_str[100], last_write_str[100], last_access_str[100];
            strftime(creation_str, sizeof(creation_str), "%Y-%m-%d %H:%M:%S", &creation_tm);
            strftime(last_write_str, sizeof(last_write_str), "%Y-%m-%d %H:%M:%S", &last_write_tm);
            strftime(last_access_str, sizeof(last_access_str), "%Y-%m-%d %H:%M:%S", &last_access_tm);
            
            // Calculate ages in hours
            auto age_creation_hours = std::chrono::duration_cast<std::chrono::hours>(
                file.creation_time.time_since_epoch()).count() / 3600;
            auto age_write_hours = std::chrono::duration_cast<std::chrono::hours>(
                file.last_write_time.time_since_epoch()).count() / 3600;
            auto age_access_hours = std::chrono::duration_cast<std::chrono::hours>(
                file.last_access_time.time_since_epoch()).count() / 3600;
            
            // Write row
            csv_file << "\"" << path_utf8 << "\"," 
                     << file.size << ","
                     << creation_str << ","
                     << last_write_str << ","
                     << last_access_str << ","
                     << age_creation_hours << ","
                     << age_write_hours << ","
                     << age_access_hours << "\n";
        }
        
        csv_file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error exporting to CSV: " << e.what() << std::endl;
        return false;
    }
}

bool FileAgeAnalyzer::export_to_json(const std::vector<AgedFileEntry>& files,
                                    const std::wstring& output_path) {
    try {
        // Convert wide string to narrow string for ofstream
        std::string narrow_path(output_path.begin(), output_path.end());
        std::ofstream json_file(narrow_path);
        
        if (!json_file.is_open()) {
            return false;
        }
        
        // Write JSON header
        json_file << "{\n  \"files\": [\n";
        
        // Write file data
        for (size_t i = 0; i < files.size(); ++i) {
            const auto& file = files[i];
            
            // Convert path to UTF-8
            std::string path_utf8(file.path.begin(), file.path.end());
            
            // Convert times to readable format
            auto creation_time_t = std::chrono::system_clock::to_time_t(file.creation_time);
            auto last_write_time_t = std::chrono::system_clock::to_time_t(file.last_write_time);
            auto last_access_time_t = std::chrono::system_clock::to_time_t(file.last_access_time);
            
            // Format times
            std::tm creation_tm, last_write_tm, last_access_tm;
#ifdef _WIN32
            localtime_s(&creation_tm, &creation_time_t);
            localtime_s(&last_write_tm, &last_write_time_t);
            localtime_s(&last_access_tm, &last_access_time_t);
#else
            localtime_r(&creation_time_t, &creation_tm);
            localtime_r(&last_write_time_t, &last_write_tm);
            localtime_r(&last_access_time_t, &last_access_tm);
#endif
            
            char creation_str[100], last_write_str[100], last_access_str[100];
            strftime(creation_str, sizeof(creation_str), "%Y-%m-%d %H:%M:%S", &creation_tm);
            strftime(last_write_str, sizeof(last_write_str), "%Y-%m-%d %H:%M:%S", &last_write_tm);
            strftime(last_access_str, sizeof(last_access_str), "%Y-%m-%d %H:%M:%S", &last_access_tm);
            
            // Calculate ages in hours
            auto age_creation_hours = std::chrono::duration_cast<std::chrono::hours>(
                file.creation_time.time_since_epoch()).count() / 3600;
            auto age_write_hours = std::chrono::duration_cast<std::chrono::hours>(
                file.last_write_time.time_since_epoch()).count() / 3600;
            auto age_access_hours = std::chrono::duration_cast<std::chrono::hours>(
                file.last_access_time.time_since_epoch()).count() / 3600;
            
            // Write row
            json_file << "    {\n";
            json_file << "      \"path\": \"" << path_utf8 << "\",\n";
            json_file << "      \"size\": " << file.size << ",\n";
            json_file << "      \"creation_time\": \"" << creation_str << "\",\n";
            json_file << "      \"last_write_time\": \"" << last_write_str << "\",\n";
            json_file << "      \"last_access_time\": \"" << last_access_str << "\",\n";
            json_file << "      \"age_since_creation_hours\": " << age_creation_hours << ",\n";
            json_file << "      \"age_since_last_write_hours\": " << age_write_hours << ",\n";
            json_file << "      \"age_since_last_access_hours\": " << age_access_hours << "\n";
            json_file << "    }" << (i < files.size() - 1 ? "," : "") << "\n";
        }
        
        // Write JSON footer
        json_file << "  ]\n}\n";
        
        json_file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error exporting to JSON: " << e.what() << std::endl;
        return false;
    }
}

FileAgeAnalyzer::AgeStatistics FileAgeAnalyzer::calculate_statistics(const std::vector<AgedFileEntry>& files) {
    AgeStatistics stats;
    
    if (files.empty()) {
        return stats;
    }
    
    stats.total_files = files.size();
    
    // Calculate total size
    uint64_t total_size = 0;
    for (const auto& file : files) {
        total_size += file.size;
    }
    stats.total_size = total_size;
    
    // Find oldest and newest files
    auto oldest_it = std::min_element(files.begin(), files.end(),
        [](const AgedFileEntry& a, const AgedFileEntry& b) {
            return a.creation_time < b.creation_time;
        });
    
    auto newest_it = std::max_element(files.begin(), files.end(),
        [](const AgedFileEntry& a, const AgedFileEntry& b) {
            return a.creation_time < b.creation_time;
        });
    
    if (oldest_it != files.end()) {
        stats.oldest_file_date = oldest_it->creation_time;
    }
    
    if (newest_it != files.end()) {
        stats.newest_file_date = newest_it->creation_time;
    }
    
    // Calculate age distribution
    std::vector<std::chrono::hours> ages;
    ages.reserve(files.size());
    
    auto now = std::chrono::system_clock::now();
    
    for (const auto& file : files) {
        auto age = now - file.creation_time;
        ages.push_back(std::chrono::duration_cast<std::chrono::hours>(age));
    }
    
    if (!ages.empty()) {
        // Calculate average age
        auto total_age = std::accumulate(ages.begin(), ages.end(), std::chrono::hours(0)).count();
        stats.average_age_hours = std::chrono::hours(total_age / ages.size());
        
        // Calculate median age
        std::sort(ages.begin(), ages.end());
        size_t median_index = ages.size() / 2;
        stats.median_age_hours = ages[median_index];
        
        // Calculate 95th percentile
        size_t p95_index = static_cast<size_t>(ages.size() * 0.95);
        stats.p95_age_hours = ages[std::min(p95_index, ages.size() - 1)];
        
        // Calculate 99th percentile
        size_t p99_index = static_cast<size_t>(ages.size() * 0.99);
        stats.p99_age_hours = ages[std::min(p99_index, ages.size() - 1)];
    }
    
    return stats;
}

bool FileAgeAnalyzer::matches_filters(const AgedFileEntry& entry, const FileAgeOptions& options) const {
    // Check size filters
    if (options.min_file_size > 0 && entry.size < options.min_file_size) {
        return false;
    }
    
    if (options.max_file_size > 0 && entry.size > options.max_file_size) {
        return false;
    }
    
    // Check extension filters
    std::wstring extension = get_file_extension(entry.path);
    
    if (!options.include_extensions.empty()) {
        bool found = false;
        for (const auto& ext : options.include_extensions) {
            if (extension == ext) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    
    if (!options.exclude_extensions.empty()) {
        for (const auto& ext : options.exclude_extensions) {
            if (extension == ext) {
                return false;
            }
        }
    }
    
    // Check path filters
    if (!options.include_paths.empty()) {
        bool found = false;
        for (const auto& path : options.include_paths) {
            if (entry.path.find(path) != std::wstring::npos) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    
    if (!options.exclude_paths.empty()) {
        for (const auto& path : options.exclude_paths) {
            if (entry.path.find(path) != std::wstring::npos) {
                return false;
            }
        }
    }
    
    return true;
}

std::wstring FileAgeAnalyzer::get_file_extension(const std::wstring& path) const {
    size_t dot_pos = path.find_last_of(L'.');
    if (dot_pos != std::wstring::npos) {
        return path.substr(dot_pos);
    }
    return L"";
}

bool FileAgeAnalyzer::process_file(const std::wstring& path, AgedFileEntry& entry) {
    try {
        // Get file size
        entry.size = std::filesystem::file_size(path);
        
        // Get file times
        auto ftime = std::filesystem::last_write_time(path);
        auto systime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime.time_since_epoch());
        entry.last_write_time = std::chrono::system_clock::time_point(systime);
        
        // For creation time and access time, we need platform-specific code
#ifdef _WIN32
        // On Windows, we can get more detailed file information
        WIN32_FILE_ATTRIBUTE_DATA attrs;
        if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &attrs)) {
            // Convert FILETIME to time_point
            uint64_t creation_time = (static_cast<uint64_t>(attrs.ftCreationTime.dwHighDateTime) << 32) | 
                                   attrs.ftCreationTime.dwLowDateTime;
            uint64_t access_time = (static_cast<uint64_t>(attrs.ftLastAccessTime.dwHighDateTime) << 32) | 
                                 attrs.ftLastAccessTime.dwLowDateTime;
            
            // FILETIME is in 100-nanosecond intervals since January 1, 1601 UTC
            // Convert to system_clock time_point
            const uint64_t EPOCH_DIFFERENCE = 116444736000000000ULL; // Diff between Windows and Unix epochs
            uint64_t creation_microseconds = (creation_time - EPOCH_DIFFERENCE) / 10;
            uint64_t access_microseconds = (access_time - EPOCH_DIFFERENCE) / 10;
            
            entry.creation_time = std::chrono::system_clock::time_point(
                std::chrono::microseconds(creation_microseconds));
            entry.last_access_time = std::chrono::system_clock::time_point(
                std::chrono::microseconds(access_microseconds));
        } else {
            // Fallback to filesystem time
            entry.creation_time = entry.last_write_time;
            entry.last_access_time = entry.last_write_time;
        }
#else
        // On Linux, we use stat to get more detailed file information
        struct stat file_stat;
        std::string narrow_path(path.begin(), path.end());
        if (stat(narrow_path.c_str(), &file_stat) == 0) {
            entry.creation_time = std::chrono::system_clock::from_time_t(file_stat.st_ctime);
            entry.last_access_time = std::chrono::system_clock::from_time_t(file_stat.st_atime);
        } else {
            // Fallback to filesystem time
            entry.creation_time = entry.last_write_time;
            entry.last_access_time = entry.last_write_time;
        }
#endif
        
        entry.path = path;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error processing file: " << e.what() << std::endl;
        return false;
    }
}