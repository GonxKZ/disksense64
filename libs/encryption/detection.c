#include "detection.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// Detect block cipher modes by analyzing data patterns
static int detect_cipher_mode(const uint8_t* data, size_t size, char** mode, double* confidence) {
    if (!data || size < 32 || !mode || !confidence) {
        return -1;
    }
    
    *mode = NULL;
    *confidence = 0.0;
    
    // Check for ECB mode patterns (repeated blocks)
    if (size >= 32) {
        int repeated_blocks = 0;
        for (size_t i = 0; i <= size - 32; i += 16) {
            for (size_t j = i + 16; j <= size - 16; j += 16) {
                if (memcmp(data + i, data + j, 16) == 0) {
                    repeated_blocks++;
                }
            }
        }
        
        // If we find many repeated blocks, it's likely ECB
        if (repeated_blocks > (int)(size / 64)) {
            *mode = strdup_safe("ECB");
            *confidence = 0.8;
            return 0;
        }
    }
    
    // Default to CBC or unknown
    *mode = strdup_safe("CBC/Unknown");
    *confidence = 0.3;
    return 0;
}

int encryption_detect_cipher_internal(const uint8_t* data, size_t size, char** cipher_name, double* confidence) {
    if (!data || size == 0 || !cipher_name || !confidence) {
        return -1;
    }
    
    *cipher_name = NULL;
    *confidence = 0.0;
    
    // Analyze data for cipher characteristics
    if (size >= 16) {
        // Check entropy - high entropy suggests strong encryption
        double entropy = 0.0;
        int freq[256] = {0};
        for (size_t i = 0; i < size; i++) {
            freq[data[i]]++;
        }
        
        for (int i = 0; i < 256; i++) {
            if (freq[i] > 0) {
                double p = (double)freq[i] / size;
                entropy -= p * log2(p);
            }
        }
        entropy /= 8.0; // Normalize to 0-1
        
        // High entropy suggests AES or similar strong cipher
        if (entropy > 0.9) {
            *cipher_name = strdup_safe("AES");
            *confidence = 0.7;
            
            // Try to detect mode
            char* mode;
            double mode_confidence;
            if (detect_cipher_mode(data, size, &mode, &mode_confidence) == 0) {
                if (mode_confidence > *confidence) {
                    free(*cipher_name);
                    *cipher_name = mode;
                    *confidence = mode_confidence;
                } else {
                    free(mode);
                }
            }
            
            return 0;
        }
        
        // Check for specific byte patterns that might indicate weaker ciphers
        // This is a simplified detection - real implementation would be more complex
        int zero_bytes = 0;
        for (size_t i = 0; i < size && i < 1024; i++) {
            if (data[i] == 0) zero_bytes++;
        }
        
        double zero_ratio = (double)zero_bytes / (size > 1024 ? 1024 : size);
        
        if (zero_ratio > 0.3) {
            *cipher_name = strdup_safe("Weak Cipher/Plaintext");
            *confidence = 0.6;
            return 0;
        }
    }
    
    // Default to unknown
    *cipher_name = strdup_safe("Unknown");
    *confidence = 0.1;
    return 0;
}

int encryption_detect_algorithm_internal(const uint8_t* data, size_t size, char** algorithm_name, double* confidence) {
    if (!data || size == 0 || !algorithm_name || !confidence) {
        return -1;
    }
    
    *algorithm_name = NULL;
    *confidence = 0.0;

    // Check for PGP ASCII armor
    if (size >= 27 && memcmp(data, "-----BEGIN PGP MESSAGE-----", 27) == 0) {
        *algorithm_name = strdup_safe("PGP (ASCII-Armored)");
        *confidence = 1.0;
        return 0;
    }

    // Check for Telegram Desktop Encrypted File
    if (size >= 4 && memcmp(data, "TDEF", 4) == 0) {
        *algorithm_name = strdup_safe("Telegram Desktop Encrypted File");
        *confidence = 1.0;
        return 0;
    }

    // Check for common encryption file signatures
    if (size >= 8) {
        // Check for LUKS
        if (memcmp(data, "LUKS", 4) == 0) {
            *algorithm_name = strdup_safe("LUKS");
            *confidence = 0.95;
            return 0;
        }
        
        // Check for TrueCrypt
        if (memcmp(data, "TRUE", 4) == 0) {
            *algorithm_name = strdup_safe("TrueCrypt");
            *confidence = 0.9;
            return 0;
        }
        
        // Check for VeraCrypt
        if (memcmp(data, "VERA", 4) == 0) {
            *algorithm_name = strdup_safe("VeraCrypt");
            *confidence = 0.9;
            return 0;
        }
        
        // Check for BitLocker
        if (memcmp(data, "-FVE-F", 6) == 0) {
            *algorithm_name = strdup_safe("BitLocker");
            *confidence = 0.9;
            return 0;
        }
    }
    
    // Check for encrypted container signatures
    if (size >= 16) {
        // Check for common encrypted container patterns
        uint32_t* header = (uint32_t*)data;
        if (header[0] == 0x00000000 && header[1] != 0x00000000) {
            *algorithm_name = strdup_safe("Encrypted Container");
            *confidence = 0.7;
            return 0;
        }
    }
    
    // Analyze entropy for encryption detection
    double entropy = 0.0;
    int freq[256] = {0};
    for (size_t i = 0; i < size && i < 4096; i++) {
        freq[data[i]]++;
    }
    
    size_t sample_size = size > 4096 ? 4096 : size;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            double p = (double)freq[i] / sample_size;
            entropy -= p * log2(p);
        }
    }
    entropy /= 8.0; // Normalize to 0-1
    
    if (entropy > 0.85) {
        *algorithm_name = strdup_safe("Symmetric Encryption");
        *confidence = 0.8;
        return 0;
    } else if (entropy > 0.7) {
        *algorithm_name = strdup_safe("Possible Encryption");
        *confidence = 0.5;
        return 0;
    }
    
    // Default to no encryption detected
    *algorithm_name = strdup_safe("None Detected");
    *confidence = 0.2;
    return 0;
}

// Public interface functions
int encryption_detect_cipher(const uint8_t* data, size_t size, char** cipher_name, double* confidence) {
    return encryption_detect_cipher_internal(data, size, cipher_name, confidence);
}

int encryption_detect_algorithm(const uint8_t* data, size_t size, char** algorithm_name, double* confidence) {
    return encryption_detect_algorithm_internal(data, size, algorithm_name, confidence);
}