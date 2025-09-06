#ifndef LIBS_COMPRESSION_COMPRESSION_UTILS_H
#define LIBS_COMPRESSION_COMPRESSION_UTILS_H

#include "compression.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Compression utility functions
typedef enum {
    COMPRESSION_UTILS_SUCCESS = 0,
    COMPRESSION_UTILS_ERROR_INVALID_PARAMETER = -1,
    COMPRESSION_UTILS_ERROR_MEMORY_ALLOCATION = -2,
    COMPRESSION_UTILS_ERROR_FILE_IO = -3,
    COMPRESSION_UTILS_ERROR_FORMAT_UNSUPPORTED = -4,
    COMPRESSION_UTILS_ERROR_CORRUPTED_DATA = -5,
    COMPRESSION_UTILS_ERROR_ENCRYPTION_DETECTED = -6,
    COMPRESSION_UTILS_ERROR_PASSWORD_REQUIRED = -7
} compression_utils_error_t;

// Memory management utilities
void* compression_utils_malloc(size_t size);
void* compression_utils_calloc(size_t nmemb, size_t size);
void* compression_utils_realloc(void* ptr, size_t size);
void compression_utils_free(void* ptr);

// String utilities
char* compression_utils_strdup(const char* str);
char* compression_utils_strndup(const char* str, size_t n);
int compression_utils_strcmp(const char* s1, const char* s2);
int compression_utils_strcasecmp(const char* s1, const char* s2);
int compression_utils_strncmp(const char* s1, const char* s2, size_t n);
int compression_utils_strncasecmp(const char* s1, const char* s2, size_t n);
size_t compression_utils_strlen(const char* str);
char* compression_utils_strcpy(char* dest, const char* src);
char* compression_utils_strncpy(char* dest, const char* src, size_t n);
char* compression_utils_strcat(char* dest, const char* src);
char* compression_utils_strncat(char* dest, const char* src, size_t n);

// File I/O utilities
FILE* compression_utils_fopen(const char* path, const char* mode);
int compression_utils_fclose(FILE* stream);
size_t compression_utils_fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t compression_utils_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
int compression_utils_fseek(FILE* stream, long offset, int whence);
long compression_utils_ftell(FILE* stream);
int compression_utils_feof(FILE* stream);
int compression_utils_ferror(FILE* stream);
void compression_utils_clearerr(FILE* stream);

// Byte order utilities
uint16_t compression_utils_le16_to_cpu(uint16_t le16);
uint32_t compression_utils_le32_to_cpu(uint32_t le32);
uint64_t compression_utils_le64_to_cpu(uint64_t le64);
uint16_t compression_utils_be16_to_cpu(uint16_t be16);
uint32_t compression_utils_be32_to_cpu(uint32_t be32);
uint64_t compression_utils_be64_to_cpu(uint64_t be64);
uint16_t compression_utils_cpu_to_le16(uint16_t cpu16);
uint32_t compression_utils_cpu_to_le32(uint32_t cpu32);
uint64_t compression_utils_cpu_to_le64(uint64_t cpu64);
uint16_t compression_utils_cpu_to_be16(uint16_t cpu16);
uint32_t compression_utils_cpu_to_be32(uint32_t cpu32);
uint64_t compression_utils_cpu_to_be64(uint64_t cpu64);

// Time conversion utilities
time_t compression_utils_dos_time_to_unix(uint16_t dos_date, uint16_t dos_time);
time_t compression_utils_filetime_to_unix(uint64_t filetime);
uint64_t compression_utils_unix_to_filetime(time_t unix_time);
uint16_t compression_utils_unix_to_dos_date(time_t unix_time);
uint16_t compression_utils_unix_to_dos_time(time_t unix_time);

// Permission utilities
char* compression_utils_permissions_to_string(uint32_t permissions);
int compression_utils_string_to_permissions(const char* perm_str, uint32_t* permissions);
int compression_utils_check_permission_violations(uint32_t permissions, int* has_violations, char** violation_details);

// Error handling utilities
const char* compression_utils_get_error_string(compression_utils_error_t error);
int compression_utils_set_error_callback(void (*callback)(compression_utils_error_t error, const char* message));
void compression_utils_clear_error_callback();

// Logging utilities
typedef enum {
    COMPRESSION_UTILS_LOG_LEVEL_DEBUG = 0,
    COMPRESSION_UTILS_LOG_LEVEL_INFO = 1,
    COMPRESSION_UTILS_LOG_LEVEL_WARNING = 2,
    COMPRESSION_UTILS_LOG_LEVEL_ERROR = 3,
    COMPRESSION_UTILS_LOG_LEVEL_CRITICAL = 4
} compression_utils_log_level_t;

int compression_utils_set_log_level(compression_utils_log_level_t level);
int compression_utils_set_log_callback(void (*callback)(compression_utils_log_level_t level, const char* message));
void compression_utils_clear_log_callback();
void compression_utils_log(compression_utils_log_level_t level, const char* format, ...);

// Buffer utilities
typedef struct {
    uint8_t* data;
    size_t size;
    size_t capacity;
    size_t position;
} compression_utils_buffer_t;

int compression_utils_buffer_init(compression_utils_buffer_t* buffer, size_t initial_capacity);
int compression_utils_buffer_resize(compression_utils_buffer_t* buffer, size_t new_capacity);
int compression_utils_buffer_append(compression_utils_buffer_t* buffer, const uint8_t* data, size_t size);
int compression_utils_buffer_prepend(compression_utils_buffer_t* buffer, const uint8_t* data, size_t size);
int compression_utils_buffer_insert(compression_utils_buffer_t* buffer, size_t position, const uint8_t* data, size_t size);
int compression_utils_buffer_remove(compression_utils_buffer_t* buffer, size_t position, size_t size);
int compression_utils_buffer_clear(compression_utils_buffer_t* buffer);
void compression_utils_buffer_free(compression_utils_buffer_t* buffer);

// CRC utilities
uint32_t compression_utils_crc32(const uint8_t* data, size_t size, uint32_t initial_crc);
uint32_t compression_utils_crc32_combine(uint32_t crc1, uint32_t crc2, size_t len2);
uint16_t compression_utils_crc16(const uint8_t* data, size_t size, uint16_t initial_crc);

// Hash utilities
uint32_t compression_utils_adler32(const uint8_t* data, size_t size, uint32_t initial_adler);
uint64_t compression_utils_fnv1a(const uint8_t* data, size_t size, uint64_t initial_hash);

// Compression ratio utilities
double compression_utils_calculate_ratio(uint64_t compressed_size, uint64_t uncompressed_size);
double compression_utils_calculate_efficiency(uint64_t compressed_size, uint64_t uncompressed_size);
double compression_utils_calculate_savings(uint64_t compressed_size, uint64_t uncompressed_size);

// Format detection utilities
compression_format_t compression_utils_detect_format_by_signature(const uint8_t* data, size_t size);
compression_format_t compression_utils_detect_format_by_extension(const char* filename);
compression_format_t compression_utils_detect_format_by_content(const uint8_t* data, size_t size);

// Path utilities
char* compression_utils_get_filename(const char* path);
char* compression_utils_get_extension(const char* path);
char* compression_utils_get_directory(const char* path);
char* compression_utils_join_paths(const char* path1, const char* path2);
int compression_utils_is_absolute_path(const char* path);
int compression_utils_normalize_path(const char* input_path, char* output_path, size_t output_size);

// Memory pattern utilities
int compression_utils_find_pattern(const uint8_t* data, size_t data_size, const uint8_t* pattern, size_t pattern_size, size_t* offset);
int compression_utils_find_multiple_patterns(const uint8_t* data, size_t data_size, const uint8_t** patterns, const size_t* pattern_sizes, size_t pattern_count, size_t* offsets);
int compression_utils_replace_pattern(uint8_t* data, size_t data_size, const uint8_t* old_pattern, size_t old_pattern_size, const uint8_t* new_pattern, size_t new_pattern_size, size_t* replaced_count);

#ifdef __cplusplus
}
#endif

#endif // LIBS_COMPRESSION_COMPRESSION_UTILS_H