#ifndef LIBS_ML_TRAINING_H
#define LIBS_ML_TRAINING_H

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

// Train logistic regression model
int ml_train_logistic_regression(const ml_training_data_t* training_data, const ml_options_t* options, ml_model_t* model);

// Train decision tree model
int ml_train_decision_tree(const ml_training_data_t* training_data, const ml_options_t* options, ml_model_t* model);

// Train random forest model
int ml_train_random_forest(const ml_training_data_t* training_data, const ml_options_t* options, ml_model_t* model);

// Train support vector machine model
int ml_train_svm(const ml_training_data_t* training_data, const ml_options_t* options, ml_model_t* model);

// Train neural network model
int ml_train_neural_network(const ml_training_data_t* training_data, const ml_options_t* options, ml_model_t* model);

// Perform cross-validation
int ml_cross_validate(const ml_training_data_t* training_data, const ml_options_t* options, double* accuracy);

#ifdef __cplusplus
}
#endif

#endif // LIBS_ML_TRAINING_H