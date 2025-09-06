#ifndef LIBS_ML_ML_H
#define LIBS_ML_ML_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// File types for classification
typedef enum {
    FILE_TYPE_UNKNOWN = 0,
    FILE_TYPE_EXECUTABLE = 1,
    FILE_TYPE_DOCUMENT = 2,
    FILE_TYPE_IMAGE = 3,
    FILE_TYPE_AUDIO = 4,
    FILE_TYPE_VIDEO = 5,
    FILE_TYPE_ARCHIVE = 6,
    FILE_TYPE_DATABASE = 7,
    FILE_TYPE_LOG = 8,
    FILE_TYPE_CONFIG = 9,
    FILE_TYPE_TEMP = 10,
    FILE_TYPE_SYSTEM = 11,
    FILE_TYPE_MALWARE = 12
} file_type_t;

// Feature vector for ML classification
typedef struct {
    double* features;
    size_t feature_count;
    char* file_path;
    file_type_t true_label;      // True label for training
    file_type_t predicted_label; // Predicted label
    double confidence;           // Confidence in prediction (0.0-1.0)
} ml_feature_vector_t;

// Machine learning model
typedef struct {
    char* model_name;
    int model_type;              // Type of ML model
    double* weights;             // Model weights
    size_t weight_count;         // Number of weights
    double bias;                 // Bias term
    int is_trained;              // Whether model is trained
    double accuracy;             // Model accuracy
} ml_model_t;

// Training data
typedef struct {
    ml_feature_vector_t* vectors;
    size_t vector_count;
    size_t capacity;
} ml_training_data_t;

// Classification result
typedef struct {
    file_type_t predicted_type;
    double confidence;
    char* explanation;
} ml_classification_result_t;

// Machine learning options
typedef struct {
    int use_metadata;            // Use file metadata features
    int use_hash_features;       // Use hash-based features
    int use_content_features;    // Use content-based features
    int use_fuzzy_hashing;       // Use fuzzy hashing features
    size_t max_features;         // Maximum number of features
    double learning_rate;        // Learning rate for training
    int max_iterations;          // Maximum training iterations
    int cross_validation_folds;  // Number of cross-validation folds
} ml_options_t;

// Initialize machine learning options with default values
void ml_options_init(ml_options_t* options);

// Free machine learning options
void ml_options_free(ml_options_t* options);

// Initialize feature vector
void ml_feature_vector_init(ml_feature_vector_t* vector);

// Free feature vector
void ml_feature_vector_free(ml_feature_vector_t* vector);

// Initialize training data
void ml_training_data_init(ml_training_data_t* data);

// Add feature vector to training data
int ml_training_data_add_vector(ml_training_data_t* data, const ml_feature_vector_t* vector);

// Free training data
void ml_training_data_free(ml_training_data_t* data);

// Initialize ML model
void ml_model_init(ml_model_t* model);

// Free ML model
void ml_model_free(ml_model_t* model);

// Initialize classification result
void ml_classification_result_init(ml_classification_result_t* result);

// Free classification result
void ml_classification_result_free(ml_classification_result_t* result);

// Extract features from file
int ml_extract_features(const char* file_path, const ml_options_t* options, ml_feature_vector_t* vector);

// Train ML model
int ml_train_model(const ml_training_data_t* training_data, const ml_options_t* options, ml_model_t* model);

// Classify file using trained model
int ml_classify_file(const char* file_path, const ml_model_t* model, const ml_options_t* options, ml_classification_result_t* result);

// Classify feature vector using trained model
int ml_classify_vector(const ml_feature_vector_t* vector, const ml_model_t* model, ml_classification_result_t* result);

// Evaluate model performance
int ml_evaluate_model(const ml_model_t* model, const ml_training_data_t* test_data, double* accuracy);

// Save trained model to file
int ml_save_model(const ml_model_t* model, const char* file_path);

// Load trained model from file
int ml_load_model(const char* file_path, ml_model_t* model);

// Get file type name
const char* ml_get_file_type_name(file_type_t type);

// Get file type description
const char* ml_get_file_type_description(file_type_t type);

// Generate training data from directory
int ml_generate_training_data(const char* directory_path, const ml_options_t* options, ml_training_data_t* training_data);

#ifdef __cplusplus
}
#endif

#endif // LIBS_ML_ML_H
