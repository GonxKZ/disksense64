#include "carving.h"
#include "signatures.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

// Helper function to duplicate memory
static void* memdup(const void* src, size_t size) {
    if (!src || size == 0) return NULL;
    void* dst = malloc(size);
    if (dst) {
        memcpy(dst, src, size);
    }
    return dst;
}

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

void carving_result_init(carving_result_t* result) {
    if (result) {
        result->files = NULL;
        result->count = 0;
        result->capacity = 0;
    }
}

int carving_result_add_file(carving_result_t* result, const carved_file_t* file) {
    if (!result || !file) {
        return -1;
    }
    
    // Reallocate if needed
    if (result->count >= result->capacity) {
        size_t new_capacity = (result->capacity == 0) ? 16 : result->capacity * 2;
        carved_file_t* new_files = (carved_file_t*)realloc(result->files, new_capacity * sizeof(carved_file_t));
        if (!new_files) {
            return -1;
        }
        result->files = new_files;
        result->capacity = new_capacity;
    }
    
    // Copy file data
    carved_file_t* new_file = &result->files[result->count];
    new_file->path = strdup_safe(file->path);
    new_file->offset = file->offset;
    new_file->size = file->size;
    new_file->extension = strdup_safe(file->extension);
    new_file->data = (uint8_t*)memdup(file->data, file->size);
    
    if (!new_file->path || !new_file->extension || (!new_file->data && file->size > 0)) {
        // Clean up on failure
        free(new_file->path);
        free(new_file->extension);
        free(new_file->data);
        return -1;
    }
    
    result->count++;
    return 0;
}

void carving_result_free(carving_result_t* result) {
    if (result) {
        if (result->files) {
            for (size_t i = 0; i < result->count; i++) {
                free(result->files[i].path);
                free(result->files[i].extension);
                free(result->files[i].data);
            }
            free(result->files);
        }
        result->files = NULL;
        result->count = 0;
        result->capacity = 0;
    }
}

// Find signature in data
static const uint8_t* find_signature(const uint8_t* data, size_t data_size, 
                                    const uint8_t* signature, size_t signature_size) {
    if (!data || !signature || data_size < signature_size) {
        return NULL;
    }
    
    for (size_t i = 0; i <= data_size - signature_size; i++) {
        if (memcmp(data + i, signature, signature_size) == 0) {
            return data + i;
        }
    }
    
    return NULL;
}

// Find file end based on footer signature or size constraints
static size_t find_file_end(const uint8_t* data, size_t data_size, size_t start_offset,
                           const file_signature_t* signature) {
    // First try to find footer signature
    if (signature->footer_signature && signature->footer_signature_length > 0) {
        const uint8_t* footer = find_signature(data + start_offset, 
                                              data_size - start_offset,
                                              signature->footer_signature,
                                              signature->footer_signature_length);
        if (footer) {
            // Include footer in file size
            return (footer - data) + signature->footer_signature_length - start_offset;
        }
    }
    
    // If no footer or not found, use size constraints
    size_t max_size = signature->max_size > 0 ? signature->max_size : data_size - start_offset;
    size_t min_size = signature->min_size > 0 ? signature->min_size : 0;
    
    // Return a reasonable size based on constraints
    if (max_size > data_size - start_offset) {
        max_size = data_size - start_offset;
    }
    
    return (max_size >= min_size) ? max_size : min_size;
}

int carving_carve_data(const uint8_t* data, size_t size, carving_result_t* result) {
    if (!data || !result) {
        return -1;
    }
    
    carving_result_init(result);
    
    // Get file signatures
    size_t signature_count;
    const file_signature_t* signatures = carving_get_signatures(&signature_count);
    
    if (!signatures || signature_count == 0) {
        return -1;
    }
    
    // Search for file signatures in data
    for (size_t i = 0; i < size; ) {
        int found = 0;
        
        // Check each signature
        for (size_t s = 0; s < signature_count; s++) {
            const file_signature_t* sig = &signatures[s];
            
            // Check if we have enough data left for the header signature
            if (i + sig->header_signature_length > size) {
                continue;
            }
            
            // Check for header signature match
            if (memcmp(data + i, sig->header_signature, sig->header_signature_length) == 0) {
                // Found a potential file, determine its end
                size_t file_size = find_file_end(data, size, i, sig);
                
                // Validate file size
                if (file_size > 0 && 
                    (sig->min_size == 0 || file_size >= sig->min_size) &&
                    (sig->max_size == 0 || file_size <= sig->max_size)) {
                    
                    // Create carved file entry
                    carved_file_t file;
                    file.path = NULL; // Will be set when saving
                    file.offset = i;
                    file.size = file_size;
                    file.extension = strdup_safe(sig->extension);
                    file.data = (uint8_t*)memdup(data + i, file_size);
                    
                    if (file.extension && (file.data || file_size == 0)) {
                        // Add to results
                        if (carving_result_add_file(result, &file) == 0) {
                            printf("Found %s file at offset %lu, size %lu\n", 
                                   sig->extension, (unsigned long)i, (unsigned long)file_size);
                        }
                    }
                    
                    // Clean up temporary file data
                    free(file.extension);
                    free(file.data);
                    
                    // Move past this file
                    i += file_size;
                    found = 1;
                    break;
                }
            }
        }
        
        // If no file found at this position, move to next byte
        if (!found) {
            i++;
        }
    }
    
    return 0;
}

int carving_carve_file(const char* path, carving_result_t* result) {
    if (!path || !result) {
        return -1;
    }
    
    // Open file
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    // Get file size
    struct stat st;
    if (fstat(fd, &st) != 0) {
        close(fd);
        return -1;
    }
    
    // For very large files, we might want to process in chunks
    // For now, load entire file into memory
    if (st.st_size > 1024 * 1024 * 100) { // 100MB limit
        printf("Warning: File is very large (%ld bytes), this might use a lot of memory\n", (long)st.st_size);
    }
    
    // Allocate buffer
    uint8_t* buffer = (uint8_t*)malloc(st.st_size);
    if (!buffer) {
        close(fd);
        return -1;
    }
    
    // Read file
    ssize_t bytes_read = read(fd, buffer, st.st_size);
    close(fd);
    
    if (bytes_read != st.st_size) {
        free(buffer);
        return -1;
    }
    
    // Carve files from data
    int ret = carving_carve_data(buffer, bytes_read, result);
    free(buffer);
    
    return ret;
}

int carving_save_files(const carving_result_t* result, const char* output_directory) {
    if (!result || !output_directory) {
        return -1;
    }
    
    // Create output directory if it doesn't exist
    struct stat st;
    if (stat(output_directory, &st) != 0) {
        // Directory doesn't exist, try to create it
#ifdef _WIN32
        if (mkdir(output_directory) != 0) {
            return -1;
        }
#else
        if (mkdir(output_directory, 0755) != 0) {
            return -1;
        }
#endif
    } else if (!S_ISDIR(st.st_mode)) {
        // Path exists but is not a directory
        return -1;
    }
    
    // Save each carved file
    for (size_t i = 0; i < result->count; i++) {
        const carved_file_t* file = &result->files[i];
        
        // Generate filename
        char filename[1024];
        snprintf(filename, sizeof(filename), "%s/carved_%08lx_%08lx.%s", 
                 output_directory, (unsigned long)file->offset, (unsigned long)file->size,
                 file->extension ? file->extension : "dat");
        
        // Open output file
        FILE* out_file = fopen(filename, "wb");
        if (!out_file) {
            printf("Failed to create output file: %s\n", filename);
            continue;
        }
        
        // Write data
        if (file->data && file->size > 0) {
            size_t written = fwrite(file->data, 1, file->size, out_file);
            if (written != file->size) {
                printf("Failed to write complete file: %s\n", filename);
            }
        }
        
        fclose(out_file);
        printf("Saved carved file: %s\n", filename);
    }
    
    return 0;
}