#ifndef LIBS_PHASH_PHASH_H
#define LIBS_PHASH_PHASH_H

#include <stdint.h>
#include <stddef.h>

// Perceptual hash for images using DCT (Discrete Cosine Transform)
// Returns a 64-bit hash where each bit represents whether a DCT coefficient
// is above or below the median value

// Compute perceptual hash for an image
// image_data: grayscale image data (8-bit per pixel)
// width: image width
// height: image height
// hash: output 64-bit perceptual hash
// Returns 0 on success, non-zero on error
int phash_image(const uint8_t* image_data, int width, int height, uint64_t* hash);

// Compute DCT-based perceptual hash
// This is a simplified implementation using a 32x32 DCT
int phash_dct(const uint8_t* image_data, int width, int height, uint64_t* hash);

// Compute hamming distance between two 64-bit hashes
// Returns the number of bits that differ
int phash_hamming_distance(uint64_t hash1, uint64_t hash2);

// Check if two hashes are similar within a threshold
// threshold: maximum hamming distance for similarity (0-64)
// Returns 1 if similar, 0 otherwise
int phash_is_similar(uint64_t hash1, uint64_t hash2, int threshold);

#endif // LIBS_PHASH_PHASH_H