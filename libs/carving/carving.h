#ifndef LIBS_CARVING_CARVING_H
#define LIBS_CARVING_CARVING_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// File signature structure
typedef struct {
    const char* extension;
    const uint8_t* header_signature;
    size_t header_signature_length;
    const uint8_t* footer_signature;
    size_t footer_signature_length;
    size_t min_size;
    size_t max_size;
} file_signature_t;

// Carved file information
typedef struct {
    char* path;
    uint64_t offset;
    size_t size;
    char* extension;
    uint8_t* data;
} carved_file_t;

// Carving result
typedef struct {
    carved_file_t* files;
    size_t count;
    size_t capacity;
} carving_result_t;

// Initialize carving result
void carving_result_init(carving_result_t* result);

// Add a carved file to the result
int carving_result_add_file(carving_result_t* result, const carved_file_t* file);

// Free carving result
void carving_result_free(carving_result_t* result);

// Carve files from raw data
// data: pointer to raw data
// size: size of data in bytes
// result: output carving result (must be freed with carving_result_free)
// Returns 0 on success, non-zero on error
int carving_carve_data(const uint8_t* data, size_t size, carving_result_t* result);

// Carve files from a file/device
// path: path to the file or device
// result: output carving result (must be freed with carving_result_free)
// Returns 0 on success, non-zero on error
int carving_carve_file(const char* path, carving_result_t* result);

// Save carved files to disk
// result: carving result containing files to save
// output_directory: directory to save files to
// Returns 0 on success, non-zero on error
int carving_save_files(const carving_result_t* result, const char* output_directory);

// Get file signatures database
const file_signature_t* carving_get_signatures(size_t* count);

#ifdef __cplusplus
}
#endif

#endif // LIBS_CARVING_CARVING_H