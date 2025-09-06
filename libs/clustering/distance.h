#ifndef LIBS_CLUSTERING_DISTANCE_H
#define LIBS_CLUSTERING_DISTANCE_H

#include "clustering.h"

#ifdef __cplusplus
extern "C" {
#endif

// Calculate Euclidean distance between two feature vectors
double clustering_euclidean_distance(const double* a, const double* b, size_t dim);

// Calculate Manhattan distance between two feature vectors
double clustering_manhattan_distance(const double* a, const double* b, size_t dim);

// Calculate cosine similarity between two feature vectors
double clustering_cosine_similarity(const double* a, const double* b, size_t dim);

// Calculate similarity between two file features
double clustering_calculate_similarity(const file_feature_t* a, const file_feature_t* b);

// Calculate distance between two file features
double clustering_calculate_distance(const file_feature_t* a, const file_feature_t* b);

#ifdef __cplusplus
}
#endif

#endif // LIBS_CLUSTERING_DISTANCE_H