#ifndef PLATFORM_FS_ENUMERATOR_H
#define PLATFORM_FS_ENUMERATOR_H

#include <windows.h>
#include <vector>
#include <string>
#include <memory>
#include <functional>

// File entry structure
struct FileEntry {
    std::wstring file_name;
    std::wstring full_path;
    uint64_t file_size;
    uint64_t creation_time;
    uint64_t last_write_time;
    uint64_t last_access_time;
    DWORD attributes;
    DWORD file_id_low;
    DWORD file_id_high;
    
    FileEntry() 
        : file_size(0), creation_time(0), last_write_time(0), 
          last_access_time(0), attributes(0), file_id_low(0), file_id_high(0) {}
};

// File enumeration callback
using FileEnumerationCallback = std::function<bool(const FileEntry&)>;

// File system enumerator
class FileSystemEnumerator {
public:
    enum EnumerationMethod {
        WIN32_WALKER,      // Traditional FindFirstFileExW
        USN_ENUMERATION,   // USN journal enumeration (requires admin)
        HYBRID_ENUMERATION // Combination of both methods
    };
    
    struct EnumerationOptions {
        bool recursive = true;
        bool include_hidden = false;
        bool include_system = false;
        std::vector<std::wstring> exclude_patterns;
        EnumerationMethod method = WIN32_WALKER;
        size_t batch_size = 1000; // Process entries in batches
        bool coalesce_directories = true; // Process directory entries together
    };
    
private:
    EnumerationOptions m_options;
    bool m_has_admin_privileges;
    
public:
    FileSystemEnumerator(const EnumerationOptions& options = EnumerationOptions());
    ~FileSystemEnumerator() = default;
    
    // Enumerate files in directory
    bool enumerate_directory(const std::wstring& path, 
                           const FileEnumerationCallback& callback);
    
    // Enumerate volume using USN journal
    bool enumerate_volume_usn(const std::wstring& volume_path,
                             const FileEnumerationCallback& callback);
    
    // Check if we have admin privileges
    bool has_admin_privileges() const { return m_has_admin_privileges; }
    
    // Set enumeration options
    void set_options(const EnumerationOptions& options) { m_options = options; }
    
    // Get current options
    const EnumerationOptions& get_options() const { return m_options; }
    
private:
    // Win32 walker implementation
    bool enumerate_win32(const std::wstring& path, 
                       const FileEnumerationCallback& callback);
    
    // USN enumeration implementation
    bool enumerate_usn(const std::wstring& volume_path,
                      const FileEnumerationCallback& callback);
    
    // Process directory entries in batches
    bool process_batch(const std::vector<FileEntry>& batch,
                       const FileEnumerationCallback& callback);
    
    // Convert WIN32_FIND_DATA to FileEntry
    FileEntry convert_find_data(const WIN32_FIND_DATAW& find_data, 
                               const std::wstring& parent_path);
    
    // Convert USN record to FileEntry
    FileEntry convert_usn_record(const USN_RECORD_V2* usn_record,
                                const std::wstring& volume_path);
    
    // Check if file should be excluded
    bool should_exclude(const FileEntry& entry) const;
    
    // Check admin privileges
    void check_admin_privileges();
};

// USN journal helper
class UsnJournalHelper {
public:
    struct JournalInfo {
        USN first_usn;
        USN next_usn;
        DWORD major_version;
        DWORD minor_version;
    };
    
private:
    HANDLE m_volume_handle;
    JournalInfo m_journal_info;
    bool m_is_valid;
    
public:
    UsnJournalHelper();
    ~UsnJournalHelper();
    
    // Open volume for USN operations
    bool open_volume(const std::wstring& volume_path);
    
    // Close volume handle
    void close_volume();
    
    // Query USN journal information
    bool query_journal_info();
    
    // Create USN journal
    bool create_journal();
    
    // Delete USN journal
    bool delete_journal();
    
    // Enumerate USN records
    bool enumerate_records(USN start_usn, USN end_usn,
                          std::function<bool(const USN_RECORD_V2*, size_t)> callback);
    
    // Get journal information
    const JournalInfo& get_journal_info() const { return m_journal_info; }
    
    // Check if helper is valid
    bool is_valid() const { return m_is_valid; }
};

#endif // PLATFORM_FS_ENUMERATOR_H