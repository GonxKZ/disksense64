#ifndef LIBS_CLUSTERING_ALGORITHMS_H
#define LIBS_CLUSTERING_ALGORITHMS_H

#include "clustering.h"

#ifdef __cplusplus
extern "C" {
#endif

// K-Means clustering algorithm
int clustering_kmeans(const file_feature_t* features, 
                     size_t count,
                     const clustering_options_t* options,
                     clustering_result_t* result);

// Hierarchical clustering algorithm
int clustering_hierarchical(const file_feature_t* features, 
                           size_t count,
                           const clustering_options_t* options,
                           clustering_result_t* result);

// DBSCAN clustering algorithm
int clustering_dbscan(const file_feature_t* features, 
                     size_t count,
                     const clustering_options_t* options,
                     clustering_result_t* result);

// Affinity Propagation clustering algorithm
int clustering_affinity_propagation(const file_feature_t* features, 
                                   size_t count,
                                   const clustering_options_t* options,
                                   clustering_result_t* result);

#ifdef __cplusplus
}
#endif

#endif // LIBS_CLUSTERING_ALGORITHMS_H