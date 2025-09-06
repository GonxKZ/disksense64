#include "tempfiles.h"
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include "core/safety/safety.h"
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

TempFileManager::TempFileManager() {
}

TempFileManager::~TempFileManager() {
}

std::vector<TempFileEntry> TempFileManager::scan_temp_files(const TempFileOptions& options) {
    std::vector<TempFileEntry> temp_files;
    
    // Get directories to scan based on options
    std::vector<std::wstring> directories_to_scan;
    
    if (options.scan_system_temp) {
        auto system_dirs = get_system_temp_directories();
        directories_to_scan.insert(directories_to_scan.end(), 
                                  system_dirs.begin(), system_dirs.end());
    }
    
    if (options.scan_user_temp) {
        auto user_dirs = get_user_temp_directories();
        directories_to_scan.insert(directories_to_scan.end(), 
                                  user_dirs.begin(), user_dirs.end());
    }
    
    if (options.scan_browser_cache) {
        auto browser_dirs = get_browser_cache_directories();
        directories_to_scan.insert(directories_to_scan.end(), 
                                  browser_dirs.begin(), browser_dirs.end());
    }
    
    // Scan each directory
    for (const auto& dir : directories_to_scan) {
        try {
            // Skip if directory doesn't exist
            if (!std::filesystem::exists(dir)) {
                continue;
            }
            
            // Recursively scan directory
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
                if (entry.is_regular_file()) {
                    TempFileEntry file_entry;
                    file_entry.path = entry.path().wstring();
                    file_entry.size = entry.file_size();
                    
                    // Get file times
                    auto ftime = entry.last_write_time();
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
                    file_entry.last_access_time = std::chrono::duration_cast<std::chrono::seconds>(
                        sctp.time_since_epoch()).count();
                    
                    file_entry.creation_time = file_entry.last_access_time; // Simplified
                    
                    // Classify file type
                    file_entry.file_type = classify_file_type(file_entry.path);
                    
                    // Check if file is old enough
                    if (is_old_enough(file_entry, options.min_age_days)) {
                        // Check exclude patterns
                        bool excluded = false;
                        for (const auto& pattern : options.exclude_patterns) {
                            if (file_entry.path.find(pattern) != std::wstring::npos) {
                                excluded = true;
                                break;
                            }
                        }
                        
                        if (!excluded) {
                            temp_files.push_back(file_entry);
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            // Skip directories we can't access
            std::wcerr << L"Warning: Could not scan directory " << dir << L": " << e.what() << std::endl;
        }
    }
    
    return temp_files;
}

uint64_t TempFileManager::estimate_savings(const std::vector<TempFileEntry>& temp_files) {
    uint64_t total_size = 0;
    for (const auto& file : temp_files) {
        total_size += file.size;
    }
    return total_size;
}

bool TempFileManager::clean_temp_files(const std::vector<TempFileEntry>& temp_files, 
                                      bool simulate_only) {
    if (simulate_only) {
        std::wcout << L"Simulation mode: Would clean " << temp_files.size() << L" temporary files" << std::endl;
        return true;
    }
    // Enforce Safety Mode: never delete if not allowed
    if (!safety::deletion_allowed()) {
        std::wcout << L"Safety Mode: deletion disabled; skipping cleanup" << std::endl;
        return false;
    }
    
    size_t cleaned_count = 0;
    uint64_t cleaned_size = 0;
    
    for (const auto& file : temp_files) {
        if (delete_file_safe(file.path)) {
            cleaned_count++;
            cleaned_size += file.size;
        }
    }
    
    std::wcout << L"Cleaned " << cleaned_count << L" temporary files (" 
               << cleaned_size << L" bytes)" << std::endl;
    
    return cleaned_count > 0;
}

std::vector<std::wstring> TempFileManager::get_system_temp_directories() {
#ifdef _WIN32
    return get_windows_temp_directories();
#else
    return get_linux_temp_directories();
#endif
}

std::vector<std::wstring> TempFileManager::get_user_temp_directories() {
    std::vector<std::wstring> user_dirs;
    
#ifdef _WIN32
    // Get user temp directory
    wchar_t user_temp[MAX_PATH];
    if (GetTempPathW(MAX_PATH, user_temp)) {
        user_dirs.push_back(std::wstring(user_temp));
    }
    
    // Get user profile directory
    wchar_t user_profile[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, SHGFP_TYPE_CURRENT, user_profile))) {
        std::wstring profile(user_profile);
        user_dirs.push_back(profile + L"\\AppData\\Local\\Temp");
        user_dirs.push_back(profile + L"\\AppData\\Roaming\\Temp");
    }
#else
    // Get user home directory
    struct passwd *pw = getpwuid(getuid());
    if (pw) {
        std::string home_dir(pw->pw_dir);
        user_dirs.push_back(std::wstring(home_dir.begin(), home_dir.end()) + L"/.cache");
        user_dirs.push_back(std::wstring(home_dir.begin(), home_dir.end()) + L"/tmp");
    }
    
    // Check environment variables
    const char* tmpdir = getenv("TMPDIR");
    if (tmpdir) {
        std::string tmp(tmpdir);
        user_dirs.push_back(std::wstring(tmp.begin(), tmp.end()));
    }
#endif
    
    return user_dirs;
}

std::vector<std::wstring> TempFileManager::get_browser_cache_directories() {
    std::vector<std::wstring> browser_dirs;
    
#ifdef _WIN32
    // Chrome/Chromium cache
    wchar_t local_appdata[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, local_appdata))) {
        std::wstring local(local_appdata);
        browser_dirs.push_back(local + L"\\Google\\Chrome\\User Data\\Default\\Cache");
        browser_dirs.push_back(local + L"\\Chromium\\User Data\\Default\\Cache");
        browser_dirs.push_back(local + L"\\Microsoft\\Edge\\User Data\\Default\\Cache");
    }
    
    // Firefox cache
    wchar_t appdata[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, appdata))) {
        std::wstring app(appdata);
        browser_dirs.push_back(app + L"\\Mozilla\\Firefox\\Profiles");
    }
#else
    // Linux browser cache directories
    struct passwd *pw = getpwuid(getuid());
    if (pw) {
        std::string home_dir(pw->pw_dir);
        std::wstring home(home_dir.begin(), home_dir.end());
        
        // Chrome/Chromium
        browser_dirs.push_back(home + L"/.cache/google-chrome");
        browser_dirs.push_back(home + L"/.cache/chromium");
        
        // Firefox
        browser_dirs.push_back(home + L"/.cache/mozilla/firefox");
        
        // Edge
        browser_dirs.push_back(home + L"/.config/microsoft-edge");
    }
#endif
    
    return browser_dirs;
}

std::wstring TempFileManager::classify_file_type(const std::wstring& path) {
    // Convert to lowercase for comparison
    std::wstring lower_path = path;
    std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), ::tolower);
    
    // Check file extension
    if (lower_path.find(L".tmp") != std::wstring::npos ||
        lower_path.find(L".temp") != std::wstring::npos) {
        return L"temp";
    }
    
    if (lower_path.find(L".log") != std::wstring::npos) {
        return L"log";
    }
    
    if (lower_path.find(L".cache") != std::wstring::npos) {
        return L"cache";
    }
    
    // Check if it's in a known cache directory
    if (lower_path.find(L"\\temp\\") != std::wstring::npos ||
        lower_path.find(L"/tmp/") != std::wstring::npos ||
        lower_path.find(L"\\cache\\") != std::wstring::npos ||
        lower_path.find(L"/cache/") != std::wstring::npos) {
        return L"cache";
    }
    
    return L"unknown";
}

bool TempFileManager::is_old_enough(const TempFileEntry& entry, uint64_t min_age_days) {
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto now_seconds = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    
    // Calculate age in days
    uint64_t age_seconds = now_seconds - entry.last_access_time;
    uint64_t age_days = age_seconds / (24 * 60 * 60);
    
    return age_days >= min_age_days;
}

std::vector<std::wstring> TempFileManager::get_windows_temp_directories() {
    std::vector<std::wstring> temp_dirs;
    
    // System temp directory
    wchar_t system_temp[MAX_PATH];
    if (GetTempPathW(MAX_PATH, system_temp)) {
        temp_dirs.push_back(std::wstring(system_temp));
    }
    
    // Windows temp directory
    temp_dirs.push_back(L"C:\\Windows\\Temp");
    
    // Prefetch directory
    temp_dirs.push_back(L"C:\\Windows\\Prefetch");
    
    return temp_dirs;
}

std::vector<std::wstring> TempFileManager::get_linux_temp_directories() {
    std::vector<std::wstring> temp_dirs;
    
    // Standard temp directories
    temp_dirs.push_back(L"/tmp");
    temp_dirs.push_back(L"/var/tmp");
    
    // Check environment variables
    const char* tmpdir = getenv("TMPDIR");
    if (tmpdir) {
        std::string tmp(tmpdir);
        temp_dirs.push_back(std::wstring(tmp.begin(), tmp.end()));
    }
    
    return temp_dirs;
}

bool TempFileManager::is_cache_file(const std::wstring& path) {
    std::wstring lower_path = path;
    std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), ::tolower);
    
    return lower_path.find(L"\\cache\\") != std::wstring::npos ||
           lower_path.find(L"/cache/") != std::wstring::npos ||
           lower_path.find(L".cache") != std::wstring::npos;
}

bool TempFileManager::is_temp_file(const std::wstring& path) {
    std::wstring lower_path = path;
    std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), ::tolower);
    
    return lower_path.find(L"\\temp\\") != std::wstring::npos ||
           lower_path.find(L"/tmp/") != std::wstring::npos ||
           lower_path.find(L".tmp") != std::wstring::npos ||
           lower_path.find(L".temp") != std::wstring::npos;
}

bool TempFileManager::is_log_file(const std::wstring& path) {
    std::wstring lower_path = path;
    std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), ::tolower);
    
    return lower_path.find(L".log") != std::wstring::npos;
}

bool TempFileManager::delete_file_safe(const std::wstring& path) {
#ifdef _WIN32
    return DeleteFileW(path.c_str()) != FALSE;
#else
    std::string narrow_path(path.begin(), path.end());
    return unlink(narrow_path.c_str()) == 0;
#endif
}

bool TempFileManager::move_to_recycle_bin(const std::wstring& path) {
#ifdef _WIN32
    // Use SHFileOperation to move to recycle bin
    SHFILEOPSTRUCTW file_op = {0};
    file_op.wFunc = FO_DELETE;
    file_op.pFrom = path.c_str();
    file_op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
    
    return SHFileOperationW(&file_op) == 0;
#else
    // On Linux, we would move to trash directory
    // For now, just delete the file
    return delete_file_safe(path);
#endif
}
