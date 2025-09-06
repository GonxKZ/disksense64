#include "compression.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

void compressed_archive_init(compressed_archive_t* archive) {
    if (archive) {
        memset(archive, 0, sizeof(compressed_archive_t));
    }
}

void compressed_entry_init(compressed_entry_t* entry) {
    if (entry) {
        memset(entry, 0, sizeof(compressed_entry_t));
    }
}

void compressed_entry_free(compressed_entry_t* entry) {
    if (entry) {
        free(entry->filename);
        free(entry->permissions);
        memset(entry, 0, sizeof(compressed_entry_t));
    }
}

int compressed_archive_add_entry(compressed_archive_t* archive, const compressed_entry_t* entry) {
    if (!archive || !entry) {
        return -1;
    }
    
    if (archive->entry_count >= archive->capacity) {
        size_t new_capacity = (archive->capacity == 0) ? 16 : archive->capacity * 2;
        compressed_entry_t* new_entries = (compressed_entry_t*)realloc(archive->entries, new_capacity * sizeof(compressed_entry_t));
        if (!new_entries) {
            return -1;
        }
        archive->entries = new_entries;
        archive->capacity = new_capacity;
    }
    
    compressed_entry_t* new_entry = &archive->entries[archive->entry_count];
    memcpy(new_entry, entry, sizeof(compressed_entry_t));
    new_entry->filename = strdup_safe(entry->filename);
    new_entry->permissions = strdup_safe(entry->permissions);

    archive->entry_count++;
    return 0;
}

void compressed_archive_free(compressed_archive_t* archive) {
    if (archive) {
        free(archive->archive_path);
        free(archive->format_name);
        if (archive->entries) {
            for (size_t i = 0; i < archive->entry_count; i++) {
                compressed_entry_free(&archive->entries[i]);
            }
            free(archive->entries);
        }
        memset(archive, 0, sizeof(compressed_archive_t));
    }
}

int compression_analyze_file(const char* file_path, const compression_options_t* options, compressed_archive_t* archive) {
    if (!file_path || !archive) {
        return -1;
    }

    struct archive *a;
    struct archive_entry *entry;
    int r;

    a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    r = archive_read_open_filename(a, file_path, 10240);
    if (r != ARCHIVE_OK) {
        archive_read_free(a);
        return -1;
    }

    compressed_archive_init(archive);
    archive->archive_path = strdup_safe(file_path);

    struct stat st;
    if (stat(file_path, &st) == 0) {
        archive->total_compressed_size = st.st_size;
    }

    int first_header = 1;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        if (first_header) {
            const char* fmt = archive_format_name(a);
            if (fmt && *fmt) {
                // Normalize to canonical short names for tests
                char lower[128];
                size_t n = strlen(fmt);
                if (n >= sizeof(lower)) n = sizeof(lower) - 1;
                for (size_t i = 0; i < n; ++i) {
                    char c = fmt[i];
                    if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
                    lower[i] = c;
                }
                lower[n] = '\0';
                if (strstr(lower, "zip")) {
                    archive->format_name = strdup_safe("zip");
                } else if (strstr(lower, "tar")) {
                    archive->format_name = strdup_safe("tar");
                } else {
                    archive->format_name = strdup_safe(fmt);
                }
            } else {
                archive->format_name = strdup_safe("");
            }
            first_header = 0;
        }
        compressed_entry_t current_entry;
        compressed_entry_init(&current_entry);

        current_entry.filename = strdup_safe(archive_entry_pathname(entry));
        current_entry.uncompressed_size = archive_entry_size(entry);
        current_entry.timestamp = archive_entry_mtime(entry);
        current_entry.is_directory = S_ISDIR(archive_entry_mode(entry));
        
        // Permissions: use POSIX rwx string; fallback if strmode is unavailable
        char perms[11];
#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
        mode_t m = archive_entry_mode(entry);
        perms[0] = S_ISDIR(m) ? 'd' : '-';
        perms[1] = (m & S_IRUSR) ? 'r' : '-';
        perms[2] = (m & S_IWUSR) ? 'w' : '-';
        perms[3] = (m & S_IXUSR) ? 'x' : '-';
        perms[4] = (m & S_IRGRP) ? 'r' : '-';
        perms[5] = (m & S_IWGRP) ? 'w' : '-';
        perms[6] = (m & S_IXGRP) ? 'x' : '-';
        perms[7] = (m & S_IROTH) ? 'r' : '-';
        perms[8] = (m & S_IWOTH) ? 'w' : '-';
        perms[9] = (m & S_IXOTH) ? 'x' : '-';
        perms[10] = '\0';
#else
        // Minimal fallback
        snprintf(perms, sizeof(perms), "-rw-r--r--");
#endif
        current_entry.permissions = strdup_safe(perms);

        archive->total_uncompressed_size += current_entry.uncompressed_size;

        compressed_archive_add_entry(archive, &current_entry);
        compressed_entry_free(&current_entry); // Free the temporary entry

        if (options && options->max_entries > 0 && archive->entry_count >= options->max_entries) {
            break;
        }
        archive_read_data_skip(a);
    }

    archive_read_close(a);
    archive_read_free(a);
    return 0;
}

int compression_extract_file(const char* archive_path, const char* entry_name, const char* output_path, const char* password) {
    if (!archive_path || !output_path) {
        return -1;
    }

    struct archive *a;
    struct archive *ext;
    struct archive_entry *entry;
    int flags;
    int r;

    flags = ARCHIVE_EXTRACT_TIME;
    flags |= ARCHIVE_EXTRACT_PERM;
    flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;

    a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);

    if ((r = archive_read_open_filename(a, archive_path, 10240))) {
        archive_read_free(a);
        archive_write_free(ext);
        return -1;
    }

    for (;;) {
        r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF)
            break;
        if (r != ARCHIVE_OK) {
            archive_read_close(a);
            archive_read_free(a);
            archive_write_free(ext);
            return -1;
        }

        if (entry_name && strcmp(archive_entry_pathname(entry), entry_name) != 0) {
            continue;
        }
        
        // Set the output path for the entry
        const char *current_file = archive_entry_pathname(entry);
        const char *dest_path = output_path;
        char new_path[4096];
        if (!entry_name) { // Extract all
            snprintf(new_path, sizeof(new_path), "%s/%s", output_path, current_file);
            dest_path = new_path;
        }
        archive_entry_set_pathname(entry, dest_path);


        r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK) {
            // Handle error
        } else {
            const void *buff;
            size_t size;
            int64_t offset;

            for (;;) {
                r = archive_read_data_block(a, &buff, &size, &offset);
                if (r == ARCHIVE_EOF)
                    break;
                if (r != ARCHIVE_OK)
                    break;
                r = archive_write_data_block(ext, buff, size, offset);
                if (r != ARCHIVE_OK) {
                    break;
                }
            }
        }
        r = archive_write_finish_entry(ext);
        if (r != ARCHIVE_OK) {
            // Handle error
        }
        
        if (entry_name) {
            break; // Extracted the single file
        }
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);
    return 0;
}

int compression_list_entries(const char* archive_path, compressed_entry_t** entries, size_t* count, const char* password) {
    if (!archive_path || !entries || !count) {
        return -1;
    }

    compressed_archive_t archive;
    if (compression_analyze_file(archive_path, NULL, &archive) != 0) {
        return -1;
    }

    *count = archive.entry_count;
    *entries = archive.entries;
    
    // Detach entries from archive so they are not freed
    archive.entries = NULL;
    archive.entry_count = 0;
    compressed_archive_free(&archive);

    return 0;
}
