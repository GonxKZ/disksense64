#include "phash.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// DCT implementation for 32x32 blocks
static void dct_32x32(const double* input, double* output) {
    // This is a simplified DCT implementation
    // In a production system, you would use a more optimized FFT-based approach
    
    const int N = 32;
    const double PI = 3.14159265358979323846;
    
    // Apply 2D DCT
    for (int u = 0; u < N; u++) {
        for (int v = 0; v < N; v++) {
            double sum = 0.0;
            
            for (int x = 0; x < N; x++) {
                for (int y = 0; y < N; y++) {
                    double cos1 = cos(PI * u * (2.0 * x + 1) / (2.0 * N));
                    double cos2 = cos(PI * v * (2.0 * y + 1) / (2.0 * N));
                    sum += input[y * N + x] * cos1 * cos2;
                }
            }
            
            double cu = (u == 0) ? sqrt(1.0 / N) : sqrt(2.0 / N);
            double cv = (v == 0) ? sqrt(1.0 / N) : sqrt(2.0 / N);
            output[v * N + u] = cu * cv * sum;
        }
    }
}

// Resize image to 32x32 using nearest neighbor
static void resize_to_32x32(const uint8_t* input, int width, int height, uint8_t* output) {
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            int src_x = (x * width) / 32;
            int src_y = (y * height) / 32;
            
            // Clamp to valid range
            src_x = (src_x < width) ? src_x : width - 1;
            src_y = (src_y < height) ? src_y : height - 1;
            
            output[y * 32 + x] = input[src_y * width + src_x];
        }
    }
}

// Calculate median of array
static double calculate_median(double* array, int size) {
    // Sort array (simplified implementation)
    for (int i = 0; i < size - 1; i++) {
        for (int j = i + 1; j < size; j++) {
            if (array[j] < array[i]) {
                double temp = array[i];
                array[i] = array[j];
                array[j] = temp;
            }
        }
    }
    
    if (size % 2 == 0) {
        return (array[size/2 - 1] + array[size/2]) / 2.0;
    } else {
        return array[size/2];
    }
}

int phash_image(const uint8_t* image_data, int width, int height, uint64_t* hash) {
    return phash_dct(image_data, width, height, hash);
}

int phash_dct(const uint8_t* image_data, int width, int height, uint64_t* hash) {
    if (!image_data || width <= 0 || height <= 0 || !hash) {
        return -1;
    }
    
    // Convert to grayscale if needed (assuming input is already grayscale)
    // Resize image to 32x32
    uint8_t resized[32 * 32];
    resize_to_32x32(image_data, width, height, resized);
    
    // Convert to double and apply DCT
    double input[32 * 32];
    for (int i = 0; i < 32 * 32; i++) {
        input[i] = (double)resized[i];
    }
    
    double dct[32 * 32];
    dct_32x32(input, dct);
    
    // Extract top-left 8x8 DCT coefficients (excluding DC coefficient)
    double coefficients[64];
    int index = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if (x == 0 && y == 0) continue; // Skip DC coefficient
            coefficients[index++] = dct[y * 32 + x];
        }
    }
    
    // Calculate median
    double median = calculate_median(coefficients, 64);
    
    // Generate hash
    *hash = 0;
    for (int i = 0; i < 64; i++) {
        if (coefficients[i] > median) {
            *hash |= (1ULL << i);
        }
    }
    
    return 0;
}

int phash_hamming_distance(uint64_t hash1, uint64_t hash2) {
    uint64_t xor_result = hash1 ^ hash2;
    int distance = 0;
    
    // Count set bits
    while (xor_result) {
        distance += xor_result & 1;
        xor_result >>= 1;
    }
    
    return distance;
}

int phash_is_similar(uint64_t hash1, uint64_t hash2, int threshold) {
    int distance = phash_hamming_distance(hash1, hash2);
    return (distance <= threshold) ? 1 : 0;
}