#include "utils.h"
#include <algorithm>
#include <cctype>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pwd.h>
#include <unistd.h>
#endif

// FileUtils implementation
file_handle_t FileUtils::open_file(const std::string& path, bool read_only) {
#ifdef _WIN32
    std::wstring wpath = StringUtils::to_wide_string(path);
    DWORD access = read_only ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE);
    DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE;
    DWORD creation = read_only ? OPEN_EXISTING : OPEN_ALWAYS;
    
    HANDLE handle = CreateFileW(
        wpath.c_str(),
        access,
        share,
        nullptr,
        creation,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    
    return handle;
#else
    int flags = read_only ? O_RDONLY : (O_RDONLY | O_WRONLY);
    int fd = open(path.c_str(), flags);
    return fd;
#endif
}

void FileUtils::close_file(file_handle_t handle) {
    if (!is_valid_handle(handle)) {
        return;
    }
    
#ifdef _WIN32
    CloseHandle(handle);
#else
    close(handle);
#endif
}

bool FileUtils::is_valid_handle(file_handle_t handle) {
#ifdef _WIN32
    return handle != INVALID_HANDLE_VALUE && handle != nullptr;
#else
    return handle != -1;
#endif
}

uint64_t FileUtils::get_file_size(file_handle_t handle) {
    if (!is_valid_handle(handle)) {
        return 0;
    }
    
#ifdef _WIN32
    LARGE_INTEGER size;
    if (GetFileSizeEx(handle, &size)) {
        return static_cast<uint64_t>(size.QuadPart);
    }
#else
    struct stat st;
    if (fstat(handle, &st) == 0) {
        return static_cast<uint64_t>(st.st_size);
    }
#endif
    
    return 0;
}

bool FileUtils::read_file_data(file_handle_t handle, void* buffer, size_t size, uint64_t offset) {
    if (!is_valid_handle(handle) || !buffer || size == 0) {
        return false;
    }
    
#ifdef _WIN32
    LARGE_INTEGER liOffset;
    liOffset.QuadPart = offset;
    if (!SetFilePointerEx(handle, liOffset, nullptr, FILE_BEGIN)) {
        return false;
    }
    
    DWORD bytesRead;
    BOOL result = ReadFile(handle, buffer, static_cast<DWORD>(size), &bytesRead, nullptr);
    return result && bytesRead == size;
#else
    off_t seekResult = lseek(handle, static_cast<off_t>(offset), SEEK_SET);
    if (seekResult == -1) {
        return false;
    }
    
    ssize_t readResult = read(handle, buffer, size);
    return readResult == static_cast<ssize_t>(size);
#endif
}

bool FileUtils::get_file_info(const std::string& path, file_info_t& info) {
#ifdef _WIN32
    std::wstring wpath = StringUtils::to_wide_string(path);
    WIN32_FILE_ATTRIBUTE_DATA attrs;
    if (!GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &attrs)) {
        return false;
    }
    
    info.name = path.substr(path.find_last_of("/\\") + 1);
    info.size = (static_cast<uint64_t>(attrs.nFileSizeHigh) << 32) | attrs.nFileSizeLow;
    info.is_directory = (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    info.creation_time = (static_cast<uint64_t>(attrs.ftCreationTime.dwHighDateTime) << 32) | attrs.ftCreationTime.dwLowDateTime;
    info.last_modified_time = (static_cast<uint64_t>(attrs.ftLastWriteTime.dwHighDateTime) << 32) | attrs.ftLastWriteTime.dwLowDateTime;
    info.last_access_time = (static_cast<uint64_t>(attrs.ftLastAccessTime.dwHighDateTime) << 32) | attrs.ftLastAccessTime.dwLowDateTime;
    info.attributes = attrs.dwFileAttributes;
#else
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return false;
    }
    
    info.name = path.substr(path.find_last_of('/') + 1);
    info.size = static_cast<uint64_t>(st.st_size);
    info.is_directory = S_ISDIR(st.st_mode);
    info.creation_time = static_cast<uint64_t>(st.st_ctime);
    info.last_modified_time = static_cast<uint64_t>(st.st_mtime);
    info.last_access_time = static_cast<uint64_t>(st.st_atime);
    info.permissions = st.st_mode;
#endif
    
    return true;
}

std::vector<file_info_t> FileUtils::list_directory(const std::string& path) {
    std::vector<file_info_t> files;
    
#ifdef _WIN32
    std::wstring wpath = StringUtils::to_wide_string(path + "\\*");
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(wpath.c_str(), &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        return files;
    }
    
    do {
        if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
            file_info_t info;
            info.name = StringUtils::to_utf8_string(findData.cFileName);
            info.size = (static_cast<uint64_t>(findData.nFileSizeHigh) << 32) | findData.nFileSizeLow;
            info.is_directory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            info.creation_time = (static_cast<uint64_t>(findData.ftCreationTime.dwHighDateTime) << 32) | findData.ftCreationTime.dwLowDateTime;
            info.last_modified_time = (static_cast<uint64_t>(findData.ftLastWriteTime.dwHighDateTime) << 32) | findData.ftLastWriteTime.dwLowDateTime;
            info.last_access_time = (static_cast<uint64_t>(findData.ftLastAccessTime.dwHighDateTime) << 32) | findData.ftLastAccessTime.dwLowDateTime;
            info.attributes = findData.dwFileAttributes;
            files.push_back(info);
        }
    } while (FindNextFileW(hFind, &findData));
    
    FindClose(hFind);
#else
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        return files;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            std::string fullPath = path + "/" + entry->d_name;
            file_info_t info;
            if (get_file_info(fullPath, info)) {
                files.push_back(info);
            }
        }
    }
    
    closedir(dir);
#endif
    
    return files;
}

char FileUtils::get_path_separator() {
    return PATH_SEPARATOR;
}

std::string FileUtils::join_paths(const std::string& path1, const std::string& path2) {
    if (path1.empty()) return path2;
    if (path2.empty()) return path1;
    
    if (path1.back() == PATH_SEPARATOR) {
        return path1 + path2;
    }
    
    return path1 + PATH_SEPARATOR_STR + path2;
}

std::string FileUtils::get_file_extension(const std::string& path) {
    size_t pos = path.find_last_of('.');
    if (pos != std::string::npos) {
        return path.substr(pos);
    }
    return "";
}

std::string FileUtils::to_platform_path(const std::string& path) {
#ifdef _WIN32
    std::string result = path;
    std::replace(result.begin(), result.end(), '/', '\\');
    return result;
#else
    std::string result = path;
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
#endif
}

// StringUtils implementation
std::wstring StringUtils::to_wide_string(const std::string& str) {
#ifdef _WIN32
    if (str.empty()) return std::wstring();
    
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
#else
    return std::wstring(str.begin(), str.end());
#endif
}

std::string StringUtils::to_utf8_string(const std::wstring& wstr) {
#ifdef _WIN32
    if (wstr.empty()) return std::string();
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
#else
    return std::string(wstr.begin(), wstr.end());
#endif
}

bool StringUtils::iequals(const std::string& a, const std::string& b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                      [](char a, char b) {
                          return std::tolower(a) == std::tolower(b);
                      });
}

std::string StringUtils::to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::vector<std::string> StringUtils::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    
    for (char c : str) {
        if (c == delimiter) {
            tokens.push_back(token);
            token.clear();
        } else {
            token += c;
        }
    }
    
    tokens.push_back(token);
    return tokens;
}

// TimeUtils implementation
uint64_t TimeUtils::get_current_time_ms() {
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    uint64_t time = (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    return time / 10000; // Convert from 100ns intervals to milliseconds
#else
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return static_cast<uint64_t>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
#endif
}

uint64_t TimeUtils::file_time_to_ms(uint64_t file_time) {
#ifdef _WIN32
    return file_time / 10000; // Convert from 100ns intervals to milliseconds
#else
    return file_time * 1000; // Convert from seconds to milliseconds
#endif
}

uint64_t TimeUtils::time_diff_ms(uint64_t start, uint64_t end) {
    return end > start ? end - start : 0;
}

// SystemUtils implementation
unsigned int SystemUtils::get_cpu_cores() {
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

uint64_t SystemUtils::get_available_memory() {
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo.ullAvailPhys;
#else
    long pages = sysconf(_SC_AVPHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return static_cast<uint64_t>(pages) * static_cast<uint64_t>(page_size);
#endif
}

uint64_t SystemUtils::get_total_memory() {
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo.ullTotalPhys;
#else
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return static_cast<uint64_t>(pages) * static_cast<uint64_t>(page_size);
#endif
}

size_t SystemUtils::get_page_size() {
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwPageSize;
#else
    return sysconf(_SC_PAGE_SIZE);
#endif
}

bool SystemUtils::is_elevated() {
#ifdef _WIN32
    BOOL isElevated = FALSE;
    HANDLE hToken = nullptr;
    
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &cbSize)) {
            isElevated = elevation.TokenIsElevated;
        }
        
        CloseHandle(hToken);
    }
    
    return isElevated;
#else
    return geteuid() == 0;
#endif
}