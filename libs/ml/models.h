#ifndef LIBS_ML_MODELS_H
#define LIBS_ML_MODELS_H

// Ensure we define _GNU_SOURCE before any system headers
#ifdef __cplusplus
// For C++ files, we need to define _GNU_SOURCE before including any system headers
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include <stdio.h>
#include "ml.h"

#ifdef __cplusplus
extern "C" {
#endif

// Save logistic regression model
int ml_save_logistic_regression_model(const ml_model_t* model, FILE* file);

// Load logistic regression model
int ml_load_logistic_regression_model(ml_model_t* model, FILE* file);

// Save decision tree model
int ml_save_decision_tree_model(const ml_model_t* model, FILE* file);

// Load decision tree model
int ml_load_decision_tree_model(ml_model_t* model, FILE* file);

// Save random forest model
int ml_save_random_forest_model(const ml_model_t* model, FILE* file);

// Load random forest model
int ml_load_random_forest_model(ml_model_t* model, FILE* file);

// Save SVM model
int ml_save_svm_model(const ml_model_t* model, FILE* file);

// Load SVM model
int ml_load_svm_model(ml_model_t* model, FILE* file);

// Save neural network model
int ml_save_neural_network_model(const ml_model_t* model, FILE* file);

// Load neural network model
int ml_load_neural_network_model(ml_model_t* model, FILE* file);

#ifdef __cplusplus
}
#endif

#endif // LIBS_ML_MODELS_H