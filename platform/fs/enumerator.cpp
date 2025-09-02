#include "enumerator.h"
#include <iostream>
#include <algorithm>
#include <set>

// FileSystemEnumerator implementation
FileSystemEnumerator::FileSystemEnumerator(const EnumerationOptions& options)
    : m_options(options), m_has_admin_privileges(false) {
    check_admin_privileges();
}

bool FileSystemEnumerator::enumerate_directory(const std::wstring& path,
                                              const FileEnumerationCallback& callback) {
    if (path.empty() || !callback) {
        return false;
    }
    
    switch (m_options.method) {
        case USN_ENUMERATION:
            if (m_has_admin_privileges) {
                return enumerate_usn(path, callback);
            }
            // Fall through to Win32 if no admin privileges
        case WIN32_WALKER:
        default:
            return enumerate_win32(path, callback);
    }
}

bool FileSystemEnumerator::enumerate_win32(const std::wstring& path,
                                         const FileEnumerationCallback& callback) {
    std::wstring search_path = path;
    if (search_path.back() != L'\\' && search_path.back() != L'/') {
        search_path += L"\\";
    }
    search_path += L"*";
    
    WIN32_FIND_DATAW find_data;
    HANDLE find_handle = FindFirstFileExW(search_path.c_str(),
                                          FindExInfoBasic,
                                          &find_data,
                                          FindExSearchNameMatch,
                                          nullptr,
                                          FIND_FIRST_EX_LARGE_FETCH);
    
    if (find_handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    std::vector<FileEntry> batch;
    batch.reserve(m_options.batch_size);
    
    bool continue_enumeration = true;
    do {
        // Skip . and ..
        if (wcscmp(find_data.cFileName, L".") == 0 || wcscmp(find_data.cFileName, L"..") == 0) {
            continue;
        }
        
        FileEntry entry = convert_find_data(find_data, path);
        
        // Apply filters
        if (should_exclude(entry)) {
            continue;
        }
        
        // Respect hidden/system file options
        if (!m_options.include_hidden && (entry.attributes & FILE_ATTRIBUTE_HIDDEN)) {
            continue;
        }
        
        if (!m_options.include_system && (entry.attributes & FILE_ATTRIBUTE_SYSTEM)) {
            continue;
        }
        
        // Add to batch
        batch.push_back(entry);
        
        // Process batch when full
        if (batch.size() >= m_options.batch_size) {
            if (!process_batch(batch, callback)) {
                continue_enumeration = false;
                break;
            }
            batch.clear();
        }
        
    } while (continue_enumeration && FindNextFileW(find_handle, &find_data));
    
    // Process remaining entries
    if (!batch.empty() && continue_enumeration) {
        process_batch(batch, callback);
    }
    
    FindClose(find_handle);
    return true;
}

bool FileSystemEnumerator::enumerate_usn(const std::wstring& volume_path,
                                         const FileEnumerationCallback& callback) {
    // This would require implementing USN journal enumeration
    // For now, fall back to Win32 enumeration
    return enumerate_win32(volume_path, callback);
}

bool FileSystemEnumerator::process_batch(const std::vector<FileEntry>& batch,
                                       const FileEnumerationCallback& callback) {
    if (m_options.coalesce_directories) {
        // Process directories first (coalescing optimization)
        std::vector<FileEntry> directories, files;
        directories.reserve(batch.size());
        files.reserve(batch.size());
        
        for (const auto& entry : batch) {
            if (entry.attributes & FILE_ATTRIBUTE_DIRECTORY) {
                directories.push_back(entry);
            } else {
                files.push_back(entry);
            }
        }
        
        // Process directories first
        for (const auto& entry : directories) {
            if (!callback(entry)) {
                return false;
            }
        }
        
        // Then process files
        for (const auto& entry : files) {
            if (!callback(entry)) {
                return false;
            }
        }
    } else {
        // Process in order
        for (const auto& entry : batch) {
            if (!callback(entry)) {
                return false;
            }
        }
    }
    
    return true;
}

FileEntry FileSystemEnumerator::convert_find_data(const WIN32_FIND_DATAW& find_data,
                                                  const std::wstring& parent_path) {
    FileEntry entry;
    entry.file_name = find_data.cFileName;
    entry.full_path = parent_path + L"\\" + find_data.cFileName;
    entry.file_size = (static_cast<uint64_t>(find_data.nFileSizeHigh) << 32) | 
                      find_data.nFileSizeLow;
    entry.attributes = find_data.dwFileAttributes;
    
    // Convert timestamps
    entry.creation_time = (static_cast<uint64_t>(find_data.ftCreationTime.dwHighDateTime) << 32) |
                          find_data.ftCreationTime.dwLowDateTime;
    entry.last_write_time = (static_cast<uint64_t>(find_data.ftLastWriteTime.dwHighDateTime) << 32) |
                            find_data.ftLastWriteTime.dwLowDateTime;
    entry.last_access_time = (static_cast<uint64_t>(find_data.ftLastAccessTime.dwHighDateTime) << 32) |
                             find_data.ftLastAccessTime.dwLowDateTime;
    
    // File ID is not available from FindFirstFileExW, we'll set it to 0
    entry.file_id_low = 0;
    entry.file_id_high = 0;
    
    return entry;
}

FileEntry FileSystemEnumerator::convert_usn_record(const USN_RECORD_V2* usn_record,
                                                   const std::wstring& volume_path) {
    FileEntry entry;
    // This would convert USN record to FileEntry
    // Implementation would depend on the specific USN record structure
    return entry;
}

bool FileSystemEnumerator::should_exclude(const FileEntry& entry) const {
    for (const auto& pattern : m_options.exclude_patterns) {
        if (entry.file_name.find(pattern) != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

void FileSystemEnumerator::check_admin_privileges() {
    HANDLE token;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elevation;
        DWORD size;
        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
            m_has_admin_privileges = (elevation.TokenIsElevated != 0);
        }
        CloseHandle(token);
    }
}

// UsnJournalHelper implementation
UsnJournalHelper::UsnJournalHelper()
    : m_volume_handle(INVALID_HANDLE_VALUE), m_is_valid(false) {
    ZeroMemory(&m_journal_info, sizeof(m_journal_info));
}

UsnJournalHelper::~UsnJournalHelper() {
    close_volume();
}

bool UsnJournalHelper::open_volume(const std::wstring& volume_path) {
    close_volume();
    
    m_volume_handle = CreateFileW(
        volume_path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    
    m_is_valid = (m_volume_handle != INVALID_HANDLE_VALUE);
    return m_is_valid;
}

void UsnJournalHelper::close_volume() {
    if (m_volume_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_volume_handle);
        m_volume_handle = INVALID_HANDLE_VALUE;
    }
    m_is_valid = false;
    ZeroMemory(&m_journal_info, sizeof(m_journal_info));
}

bool UsnJournalHelper::query_journal_info() {
    if (!m_is_valid) {
        return false;
    }
    
    DWORD bytes_returned;
    USN_JOURNAL_DATA journal_data;
    
    BOOL result = DeviceIoControl(
        m_volume_handle,
        FSCTL_QUERY_USN_JOURNAL,
        nullptr, 0,
        &journal_data, sizeof(journal_data),
        &bytes_returned,
        nullptr
    );
    
    if (result) {
        m_journal_info.first_usn = journal_data.FirstUsn;
        m_journal_info.next_usn = journal_data.NextUsn;
        m_journal_info.major_version = journal_data.MajorVersion;
        m_journal_info.minor_version = journal_data.MinorVersion;
    }
    
    return result;
}

bool UsnJournalHelper::create_journal() {
    if (!m_is_valid) {
        return false;
    }
    
    CREATE_USN_JOURNAL_DATA create_data = {0};
    create_data.MaximumSize = 0; // Use default
    create_data.AllocationDelta = 0; // Use default
    
    DWORD bytes_returned;
    
    return DeviceIoControl(
        m_volume_handle,
        FSCTL_CREATE_USN_JOURNAL,
        &create_data, sizeof(create_data),
        nullptr, 0,
        &bytes_returned,
        nullptr
    );
}

bool UsnJournalHelper::delete_journal() {
    if (!m_is_valid) {
        return false;
    }
    
    DELETE_USN_JOURNAL_DATA delete_data = {0};
    delete_data.DeleteFlags = USN_DELETE_FLAG_NOTIFY; // Don't delete, just notify
    
    DWORD bytes_returned;
    
    return DeviceIoControl(
        m_volume_handle,
        FSCTL_DELETE_USN_JOURNAL,
        &delete_data, sizeof(delete_data),
        nullptr, 0,
        &bytes_returned,
        nullptr
    );
}

bool UsnJournalHelper::enumerate_records(USN start_usn, USN end_usn,
                                         std::function<bool(const USN_RECORD_V2*, size_t)> callback) {
    if (!m_is_valid || !callback) {
        return false;
    }
    
    // Allocate buffer for reading USN records
    const size_t buffer_size = 64 * 1024; // 64KB buffer
    std::vector<char> buffer(buffer_size);
    
    READ_USN_JOURNAL_DATA read_data = {0};
    read_data.StartUsn = start_usn;
    read_data.ReasonMask = 0xFFFFFFFF; // All reasons
    read_data.ReturnOnlyOnClose = FALSE;
    read_data.Timeout = 0;
    read_data.BytesToWaitFor = 0;
    read_data.UsnJournalID = m_journal_info.first_usn; // This should be the actual journal ID
    
    DWORD bytes_returned;
    
    while (read_data.StartUsn < end_usn) {
        BOOL result = DeviceIoControl(
            m_volume_handle,
            FSCTL_READ_USN_JOURNAL,
            &read_data, sizeof(read_data),
            buffer.data(), static_cast<DWORD>(buffer_size),
            &bytes_returned,
            nullptr
        );
        
        if (!result || bytes_returned == 0) {
            break;
        }
        
        // Process USN records
        const char* current = buffer.data() + sizeof(USN);
        const char* end = buffer.data() + bytes_returned;
        
        while (current < end) {
            const USN_RECORD_V2* record = reinterpret_cast<const USN_RECORD_V2*>(current);
            
            if (record->RecordLength == 0) {
                break;
            }
            
            // Call callback
            if (!callback(record, bytes_returned)) {
                return false;
            }
            
            // Move to next record
            current += record->RecordLength;
        }
        
        // Update start USN for next iteration
        read_data.StartUsn = *reinterpret_cast<const USN*>(buffer.data());
    }
    
    return true;
}