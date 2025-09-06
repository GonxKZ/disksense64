#ifndef LIBS_ML_FEATURES_H
#define LIBS_ML_FEATURES_H

// Ensure we define _GNU_SOURCE before any system headers
#ifdef __cplusplus
// For C++ files, we need to define _GNU_SOURCE before including any system headers
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include <stddef.h>
#include "ml.h"

#ifdef __cplusplus
extern "C" {
#endif

// Extract metadata features
int ml_extract_metadata_features(const char* file_path, double* features, size_t* feature_count);

// Extract hash features
int ml_extract_hash_features(const char* file_path, double* features, size_t* feature_count);

// Extract content features
int ml_extract_content_features(const char* file_path, double* features, size_t* feature_count);

// Extract fuzzy hash features
int ml_extract_fuzzy_features(const char* file_path, double* features, size_t* feature_count);

// Normalize feature vector
int ml_normalize_features(double* features, size_t feature_count);

#ifdef __cplusplus
}
#endif

#endif // LIBS_ML_FEATURES_H