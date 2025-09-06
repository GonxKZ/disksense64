#include "entropy.h"
#include <math.h>

double encryption_calculate_entropy_internal(const uint8_t* data, size_t size) {
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

// Public interface function
double encryption_calculate_entropy(const uint8_t* data, size_t size) {
    return encryption_calculate_entropy_internal(data, size);
}