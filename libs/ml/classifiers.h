#ifndef LIBS_ML_CLASSIFIERS_H
#define LIBS_ML_CLASSIFIERS_H

// Ensure we define _GNU_SOURCE before any system headers
#ifdef __cplusplus
// For C++ files, we need to define _GNU_SOURCE before including any system headers
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include "ml.h"

#ifdef __cplusplus
extern "C" {
#endif

// Logistic regression classifier
int ml_logistic_regression_classify(const ml_feature_vector_t* vector, const ml_model_t* model, ml_classification_result_t* result);

// Decision tree classifier
int ml_decision_tree_classify(const ml_feature_vector_t* vector, const ml_model_t* model, ml_classification_result_t* result);

// Random forest classifier
int ml_random_forest_classify(const ml_feature_vector_t* vector, const ml_model_t* model, ml_classification_result_t* result);

// Support vector machine classifier
int ml_svm_classify(const ml_feature_vector_t* vector, const ml_model_t* model, ml_classification_result_t* result);

// Neural network classifier
int ml_neural_network_classify(const ml_feature_vector_t* vector, const ml_model_t* model, ml_classification_result_t* result);

#ifdef __cplusplus
}
#endif

#endif // LIBS_ML_CLASSIFIERS_H