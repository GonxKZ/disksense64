#include "encryption.h"
#include "detection.h"
#include "ciphers.h"
#include "entropy.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>
#include <ctype.h>

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

void encryption_result_init(encryption_result_t* result) {
    if (result) {
        memset(result, 0, sizeof(encryption_result_t));
    }
}

void encryption_result_free(encryption_result_t* result) {
    if (result) {
        free(result->file_path);
        free(result->cipher_type);
        free(result->encryption_algorithm);
        free(result->mode_of_operation);
        memset(result, 0, sizeof(encryption_result_t));
    }
}

void encryption_options_init(encryption_options_t* options) {
    if (options) {
        memset(options, 0, sizeof(encryption_options_t));
        options->check_entropy = 1;
        options->detect_ciphers = 1;
        options->detect_compression = 1;
        options->check_headers = 1;
        options->deep_analysis = 0;
        options->entropy_threshold = 0.8;
        options->sample_size = 4096;
    }
}

void encryption_options_free(encryption_options_t* options) {
    if (options) {
        free(options->password);
        memset(options, 0, sizeof(encryption_options_t));
    }
}

void cipher_info_init(cipher_info_t* info) {
    if (info) {
        memset(info, 0, sizeof(cipher_info_t));
    }
}

void cipher_info_free(cipher_info_t* info) {
    if (info) {
        free(info->name);
        free(info->description);
        memset(info, 0, sizeof(cipher_info_t));
    }
}

int encryption_analyze_file(const char* file_path,
                          const encryption_options_t* options,
                          encryption_result_t* result) {
    if (!file_path || !result) {
        return -1;
    }
    
    encryption_result_init(result);
    
    // Check if file exists
    if (access(file_path, R_OK) != 0) {
        return -1;
    }
    
    // Get file statistics
    struct stat st;
    if (stat(file_path, &st) != 0) {
        return -1;
    }
    
    // Open file
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Determine sample size
    size_t sample_size = options ? options->sample_size : 4096;
    if (sample_size > (size_t)st.st_size) {
        sample_size = st.st_size;
    }
    
    // Allocate buffer
    uint8_t* buffer = (uint8_t*)malloc(sample_size);
    if (!buffer) {
        fclose(file);
        return -1;
    }
    
    // Read file data
    size_t bytes_read = fread(buffer, 1, sample_size, file);
    fclose(file);
    
    if (bytes_read == 0) {
        free(buffer);
        return -1;
    }
    
    // Fill basic result information
    result->file_path = strdup_safe(file_path);
    result->confidence = 0.0;
    
    // Analyze data
    int ret = encryption_analyze_data(buffer, bytes_read, options, result);
    
    free(buffer);
    return ret;
}

int encryption_analyze_data(const void* data,
                          size_t size,
                          const encryption_options_t* options,
                          encryption_result_t* result) {
    if (!data || size == 0 || !result) {
        return -1;
    }
    
    const uint8_t* byte_data = (const uint8_t*)data;
    
    // Calculate entropy
    if (!options || options->check_entropy) {
        result->entropy = encryption_calculate_entropy(byte_data, size);
        result->is_encrypted = (result->entropy >= (options ? options->entropy_threshold : 0.8)) ? 1 : 0;
    }
    
    // Check for compression
    if (!options || options->detect_compression) {
        encryption_is_compressed(byte_data, size, &result->is_compressed);
    }
    
    // Analyze data for encryption
    double algorithm_confidence = 0.0;
    if (!options || options->detect_ciphers) {
        encryption_detect_algorithm(byte_data, size, &result->encryption_algorithm, &algorithm_confidence);
        result->confidence = algorithm_confidence;
    }

    // If no specific algorithm was found, check for generic characteristics
    if (algorithm_confidence < 0.9) {
        double cipher_confidence = 0.0;
        if (!options || options->detect_ciphers) {
            encryption_detect_cipher(byte_data, size, &result->cipher_type, &cipher_confidence);
        }

        // Combine confidences
        double entropy_confidence = result->entropy;
        result->confidence = fmax(algorithm_confidence, fmax(cipher_confidence, entropy_confidence));

        if (result->entropy > (options ? options->entropy_threshold : 0.8)) {
            result->is_encrypted = 1;
        }
    } else {
        result->is_encrypted = 1;
    }
    
    return 0;
}

int encryption_analyze_directory(const char* directory_path,
                               const encryption_options_t* options,
                               encryption_result_t** results,
                               size_t* count) {
    if (!directory_path || !results || !count) {
        return -1;
    }
    
    *results = NULL;
    *count = 0;
    
    // Open directory
    DIR* dir = opendir(directory_path);
    if (!dir) {
        return -1;
    }
    
    // Count files first
    size_t file_count = 0;
    struct dirent* entry;
    
    while ((entry = readdir(dir)) != NULL) {
        // Skip current and parent directory
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Build full path
        size_t path_len = strlen(directory_path);
        size_t name_len = strlen(entry->d_name);
        char* full_path = (char*)malloc(path_len + name_len + 2);
        if (!full_path) {
            continue;
        }
        
        sprintf(full_path, "%s/%s", directory_path, entry->d_name);
        
        // Check if it's a regular file
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            file_count++;
        }
        
        free(full_path);
    }
    
    // Allocate results array
    *results = (encryption_result_t*)malloc(file_count * sizeof(encryption_result_t));
    if (!*results) {
        closedir(dir);
        return -1;
    }
    
    // Process files
    rewinddir(dir);
    size_t index = 0;
    
    while ((entry = readdir(dir)) != NULL && index < file_count) {
        // Skip current and parent directory
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Build full path
        size_t path_len = strlen(directory_path);
        size_t name_len = strlen(entry->d_name);
        char* full_path = (char*)malloc(path_len + name_len + 2);
        if (!full_path) {
            continue;
        }
        
        sprintf(full_path, "%s/%s", directory_path, entry->d_name);
        
        // Check if it's a regular file
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            // Analyze file
            encryption_result_init(&(*results)[index]);
            if (encryption_analyze_file(full_path, options, &(*results)[index]) == 0) {
                index++;
            } else {
                encryption_result_free(&(*results)[index]);
            }
        }
        
        free(full_path);
    }
    
    closedir(dir);
    *count = index;
    
    return 0;
}

double encryption_calculate_entropy(const uint8_t* data, size_t size) {
    if (!data || size == 0) {
        return 0.0;
    }
    
    // Count byte frequencies
    size_t frequency[256] = {0};
    for (size_t i = 0; i < size; i++) {
        frequency[data[i]]++;
    }
    
    // Calculate entropy
    double entropy = 0.0;
    for (int i = 0; i < 256; i++) {
        if (frequency[i] > 0) {
            double p = (double)frequency[i] / size;
            entropy -= p * log2(p);
        }
    }
    
    // Normalize to 0-1 range (max entropy for 8-bit data is 8)
    return entropy / 8.0;
}

int encryption_is_compressed(const uint8_t* data, size_t size, int* is_compressed) {
    if (!data || size == 0 || !is_compressed) {
        return -1;
    }
    
    *is_compressed = 0;
    
    // Check for common compression signatures
    if (size >= 4) {
        // Check for ZIP signature
        if (data[0] == 0x50 && data[1] == 0x4B && data[2] == 0x03 && data[3] == 0x04) {
            *is_compressed = 1;
            return 0;
        }
        
        // Check for GZIP signature
        if (data[0] == 0x1F && data[1] == 0x8B) {
            *is_compressed = 1;
            return 0;
        }
        
        // Check for BZIP2 signature
        if (data[0] == 0x42 && data[1] == 0x5A && data[2] == 0x68) {
            *is_compressed = 1;
            return 0;
        }
        
        // Check for XZ signature
        if (data[0] == 0xFD && data[1] == 0x37 && data[2] == 0x7A && data[3] == 0x58 && data[4] == 0x5A && data[5] == 0x00) {
            *is_compressed = 1;
            return 0;
        }
    }
    
    if (size >= 8) {
        // Check for CAB signature
        if (memcmp(data, "MSCF", 4) == 0) {
            *is_compressed = 1;
            return 0;
        }
        // Check for DEB signature
        if (memcmp(data, "!<arch>\n", 8) == 0) {
            *is_compressed = 1;
            return 0;
        }
    }

    // Check for RAR signature
    if (size >= 7) {
        if (data[0] == 0x52 && data[1] == 0x61 && data[2] == 0x72 && data[3] == 0x21 && 
            data[4] == 0x1A && data[5] == 0x07 && data[6] == 0x00) {
            *is_compressed = 1;
            return 0;
        }
    }
    
    // Check for 7Z signature
    if (size >= 6) {
        if (data[0] == 0x37 && data[1] == 0x7A && data[2] == 0xBC && 
            data[3] == 0xAF && data[4] == 0x27 && data[5] == 0x1C) {
            *is_compressed = 1;
            return 0;
        }
    }

    // Check for DMG signature
    if (size >= 7 && memcmp(data, "\x78\x01\x73\x0d\x62\x62\x60", 7) == 0) {
        *is_compressed = 1;
        return 0;
    }

    // Check for Z signature
    if (size >= 2 && (memcmp(data, "\x1f\x9d", 2) == 0 || memcmp(data, "\x1f\xa0", 2) == 0)) {
        *is_compressed = 1;
        return 0;
    }

    return 0;
}

const char* encryption_get_cipher_description(const char* cipher_name) {
    if (!cipher_name) {
        return NULL;
    }
    
    // Return descriptions for common ciphers
    if (strcasecmp(cipher_name, "AES") == 0) {
        return "Advanced Encryption Standard - symmetric block cipher";
    } else if (strcasecmp(cipher_name, "DES") == 0) {
        return "Data Encryption Standard - symmetric block cipher (deprecated)";
    } else if (strcasecmp(cipher_name, "3DES") == 0) {
        return "Triple DES - symmetric block cipher using three DES operations";
    } else if (strcasecmp(cipher_name, "Blowfish") == 0) {
        return "Symmetric block cipher with variable key length";
    } else if (strcasecmp(cipher_name, "Twofish") == 0) {
        return "Symmetric block cipher, successor to Blowfish";
    } else if (strcasecmp(cipher_name, "RC4") == 0) {
        return "Stream cipher with variable key size";
    } else if (strcasecmp(cipher_name, "ChaCha20") == 0) {
        return "Stream cipher designed for high-speed operation";
    } else if (strcasecmp(cipher_name, "Salsa20") == 0) {
        return "Stream cipher, predecessor to ChaCha20";
    }
    
    return "Unknown cipher";
}

const char* encryption_get_entropy_description(double entropy) {
    if (entropy < 0.0) entropy = 0.0;
    if (entropy > 1.0) entropy = 1.0;
    
    if (entropy >= 0.9) {
        return "Highly random - likely encrypted or compressed";
    } else if (entropy >= 0.8) {
        return "Moderately random - possibly encrypted or compressed";
    } else if (entropy >= 0.7) {
        return "Somewhat random - contains some structured data";
    } else if (entropy >= 0.5) {
        return "Moderately structured - likely unencrypted data";
    } else {
        return "Highly structured - definitely unencrypted data";
    }
}