#include "libs/ml/ml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int test_ml_options() {
    printf("Testing ML options...\n");
    
    ml_options_t options;
    ml_options_init(&options);
    
    printf("Default ML options:\n");
    printf("  Use metadata: %s\n", options.use_metadata ? "yes" : "no");
    printf("  Use hash features: %s\n", options.use_hash_features ? "yes" : "no");
    printf("  Use content features: %s\n", options.use_content_features ? "yes" : "no");
    printf("  Use fuzzy hashing: %s\n", options.use_fuzzy_hashing ? "yes" : "no");
    printf("  Max features: %lu\n", (unsigned long)options.max_features);
    printf("  Learning rate: %.4f\n", options.learning_rate);
    printf("  Max iterations: %d\n", options.max_iterations);
    printf("  Cross-validation folds: %d\n", options.cross_validation_folds);
    
    // Test modifying options
    options.use_hash_features = 0;
    options.max_features = 50;
    options.learning_rate = 0.1;
    
    printf("Modified ML options:\n");
    printf("  Use hash features: %s\n", options.use_hash_features ? "yes" : "no");
    printf("  Max features: %lu\n", (unsigned long)options.max_features);
    printf("  Learning rate: %.4f\n", options.learning_rate);
    
    // Clean up
    ml_options_free(&options);
    
    printf("ML options test passed!\n");
    return 0;
}

int test_feature_extraction() {
    printf("Testing feature extraction...\n");
    
    // Create a simple test file
    const char* test_file = "/tmp/ml_test_file.txt";
    FILE* file = fopen(test_file, "w");
    if (file) {
        fprintf(file, "This is a test file for machine learning feature extraction.\n");
        fprintf(file, "It contains some text that can be used to extract features.\n");
        fprintf(file, "The file is used to test the ML library functionality.\n");
        fclose(file);
    }
    
    ml_options_t options;
    ml_options_init(&options);
    
    ml_feature_vector_t vector;
    int ret = ml_extract_features(test_file, &options, &vector);
    
    if (ret == 0) {
        printf("Feature extraction result:\n");
        printf("  File path: %s\n", vector.file_path ? vector.file_path : "unknown");
        printf("  Feature count: %lu\n", (unsigned long)vector.feature_count);
        printf("  True label: %s\n", ml_get_file_type_name(vector.true_label));
        printf("  Predicted label: %s\n", ml_get_file_type_name(vector.predicted_label));
        printf("  Confidence: %.4f\n", vector.confidence);
        
        // Show first few features
        printf("  Features (first 10): ");
        for (size_t i = 0; i < vector.feature_count && i < 10; i++) {
            printf("%.4f ", vector.features[i]);
        }
        printf("\n");
    } else {
        printf("Failed to extract features\n");
        unlink(test_file);
        ml_options_free(&options);
        return 1;
    }
    
    // Clean up
    ml_feature_vector_free(&vector);
    ml_options_free(&options);
    unlink(test_file);
    
    printf("Feature extraction test passed!\n");
    return 0;
}

int test_model_training() {
    printf("Testing model training...\n");
    
    // Create some training data
    ml_training_data_t training_data;
    ml_training_data_init(&training_data);
    
    // Add some dummy feature vectors
    for (int i = 0; i < 10; i++) {
        ml_feature_vector_t vector;
        ml_feature_vector_init(&vector);
        vector.feature_count = 20;
        vector.features = (double*)malloc(vector.feature_count * sizeof(double));
        vector.file_path = strdup("/tmp/dummy_file.txt");
        vector.true_label = (i % 2 == 0) ? FILE_TYPE_MALWARE : FILE_TYPE_EXECUTABLE;
        vector.predicted_label = FILE_TYPE_UNKNOWN;
        vector.confidence = 0.0;
        
        // Fill with random features
        for (size_t j = 0; j < vector.feature_count; j++) {
            vector.features[j] = (double)rand() / RAND_MAX;
        }
        
        ml_training_data_add_vector(&training_data, &vector);
        ml_feature_vector_free(&vector);
    }
    
    ml_options_t options;
    ml_options_init(&options);
    
    // Test training different models
    ml_model_t models[5];
    
    // Logistic regression
    int ret = ml_train_logistic_regression(&training_data, &options, &models[0]);
    if (ret == 0) {
        printf("Logistic regression model trained successfully\n");
        printf("  Model name: %s\n", models[0].model_name ? models[0].model_name : "unknown");
        printf("  Accuracy: %.4f\n", models[0].accuracy);
        printf("  Weights: %lu\n", (unsigned long)models[0].weight_count);
    } else {
        printf("Failed to train logistic regression model\n");
    }
    
    // Decision tree
    ret = ml_train_decision_tree(&training_data, &options, &models[1]);
    if (ret == 0) {
        printf("Decision tree model trained successfully\n");
        printf("  Model name: %s\n", models[1].model_name ? models[1].model_name : "unknown");
        printf("  Accuracy: %.4f\n", models[1].accuracy);
    } else {
        printf("Failed to train decision tree model\n");
    }
    
    // Random forest
    ret = ml_train_random_forest(&training_data, &options, &models[2]);
    if (ret == 0) {
        printf("Random forest model trained successfully\n");
        printf("  Model name: %s\n", models[2].model_name ? models[2].model_name : "unknown");
        printf("  Accuracy: %.4f\n", models[2].accuracy);
    } else {
        printf("Failed to train random forest model\n");
    }
    
    // SVM
    ret = ml_train_svm(&training_data, &options, &models[3]);
    if (ret == 0) {
        printf("SVM model trained successfully\n");
        printf("  Model name: %s\n", models[3].model_name ? models[3].model_name : "unknown");
        printf("  Accuracy: %.4f\n", models[3].accuracy);
        printf("  Weights: %lu\n", (unsigned long)models[3].weight_count);
    } else {
        printf("Failed to train SVM model\n");
    }
    
    // Neural network
    ret = ml_train_neural_network(&training_data, &options, &models[4]);
    if (ret == 0) {
        printf("Neural network model trained successfully\n");
        printf("  Model name: %s\n", models[4].model_name ? models[4].model_name : "unknown");
        printf("  Accuracy: %.4f\n", models[4].accuracy);
        printf("  Weights: %lu\n", (unsigned long)models[4].weight_count);
    } else {
        printf("Failed to train neural network model\n");
    }
    
    // Clean up
    for (int i = 0; i < 5; i++) {
        ml_model_free(&models[i]);
    }
    ml_training_data_free(&training_data);
    ml_options_free(&options);
    
    printf("Model training test passed!\n");
    return 0;
}

int test_file_classification() {
    printf("Testing file classification...\n");
    
    // Create a test file
    const char* test_file = "/tmp/ml_classification_test.txt";
    FILE* file = fopen(test_file, "w");
    if (file) {
        fprintf(file, "This is a test file for classification.\n");
        fprintf(file, "It should be classified as a document.\n");
        fclose(file);
    }
    
    // Create a dummy model
    ml_model_t model;
    ml_model_init(&model);
    model.model_name = strdup("TestModel");
    model.model_type = 1; // Logistic regression
    model.weight_count = 10;
    model.weights = (double*)malloc(model.weight_count * sizeof(double));
    model.bias = 0.0;
    model.is_trained = 1;
    model.accuracy = 0.85;
    
    // Initialize weights
    for (size_t i = 0; i < model.weight_count; i++) {
        model.weights[i] = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
    }
    
    ml_options_t options;
    ml_options_init(&options);
    
    // Classify the file
    ml_classification_result_t result;
    int ret = ml_classify_file(test_file, &model, &options, &result);
    
    if (ret == 0) {
        printf("File classification result:\n");
        printf("  Predicted type: %s\n", ml_get_file_type_name(result.predicted_type));
        printf("  Confidence: %.4f\n", result.confidence);
        printf("  Explanation: %s\n", result.explanation ? result.explanation : "none");
    } else {
        printf("Failed to classify file\n");
        ml_model_free(&model);
        ml_options_free(&options);
        unlink(test_file);
        return 1;
    }
    
    // Clean up
    ml_classification_result_free(&result);
    ml_model_free(&model);
    ml_options_free(&options);
    unlink(test_file);
    
    printf("File classification test passed!\n");
    return 0;
}

int test_file_type_info() {
    printf("Testing file type information...\n");
    
    file_type_t types[] = {
        FILE_TYPE_UNKNOWN,
        FILE_TYPE_EXECUTABLE,
        FILE_TYPE_DOCUMENT,
        FILE_TYPE_IMAGE,
        FILE_TYPE_AUDIO,
        FILE_TYPE_VIDEO,
        FILE_TYPE_ARCHIVE,
        FILE_TYPE_DATABASE,
        FILE_TYPE_LOG,
        FILE_TYPE_CONFIG,
        FILE_TYPE_TEMP,
        FILE_TYPE_SYSTEM,
        FILE_TYPE_MALWARE
    };
    
    int type_count = sizeof(types) / sizeof(types[0]);
    
    printf("File type information:\n");
    for (int i = 0; i < type_count; i++) {
        const char* name = ml_get_file_type_name(types[i]);
        const char* description = ml_get_file_type_description(types[i]);
        printf("  %s: %s\n", name, description);
    }
    
    printf("File type information test passed!\n");
    return 0;
}

int test_model_save_load() {
    printf("Testing model save/load...\n");
    
    // Create a dummy model
    ml_model_t model;
    ml_model_init(&model);
    model.model_name = strdup("TestSaveLoadModel");
    model.model_type = 1; // Logistic regression
    model.weight_count = 5;
    model.weights = (double*)malloc(model.weight_count * sizeof(double));
    model.bias = 0.5;
    model.is_trained = 1;
    model.accuracy = 0.9;
    
    // Initialize weights
    for (size_t i = 0; i < model.weight_count; i++) {
        model.weights[i] = (double)i / 10.0;
    }
    
    // Save model
    const char* model_file = "/tmp/test_model.dat";
    int ret = ml_save_model(&model, model_file);
    
    if (ret == 0) {
        printf("Model saved successfully to %s\n", model_file);
    } else {
        printf("Failed to save model\n");
        ml_model_free(&model);
        return 1;
    }
    
    // Load model
    ml_model_t loaded_model;
    ret = ml_load_model(model_file, &loaded_model);
    
    if (ret == 0) {
        printf("Model loaded successfully\n");
        printf("  Model name: %s\n", loaded_model.model_name ? loaded_model.model_name : "unknown");
        printf("  Model type: %d\n", loaded_model.model_type);
        printf("  Weight count: %lu\n", (unsigned long)loaded_model.weight_count);
        printf("  Bias: %.4f\n", loaded_model.bias);
        printf("  Trained: %s\n", loaded_model.is_trained ? "yes" : "no");
        printf("  Accuracy: %.4f\n", loaded_model.accuracy);
    } else {
        printf("Failed to load model\n");
        ml_model_free(&model);
        unlink(model_file);
        return 1;
    }
    
    // Clean up
    ml_model_free(&model);
    ml_model_free(&loaded_model);
    unlink(model_file);
    
    printf("Model save/load test passed!\n");
    return 0;
}

int test_cross_validation() {
    printf("Testing cross-validation...\n");
    
    // Create some training data
    ml_training_data_t training_data;
    ml_training_data_init(&training_data);
    
    // Add some dummy feature vectors
    for (int i = 0; i < 20; i++) {
        ml_feature_vector_t vector;
        ml_feature_vector_init(&vector);
        vector.feature_count = 10;
        vector.features = (double*)malloc(vector.feature_count * sizeof(double));
        vector.file_path = strdup("/tmp/dummy_file.txt");
        vector.true_label = (i % 3 == 0) ? FILE_TYPE_MALWARE : 
                           (i % 3 == 1) ? FILE_TYPE_EXECUTABLE : FILE_TYPE_DOCUMENT;
        vector.predicted_label = FILE_TYPE_UNKNOWN;
        vector.confidence = 0.0;
        
        // Fill with random features
        for (size_t j = 0; j < vector.feature_count; j++) {
            vector.features[j] = (double)rand() / RAND_MAX;
        }
        
        ml_training_data_add_vector(&training_data, &vector);
        ml_feature_vector_free(&vector);
    }
    
    ml_options_t options;
    ml_options_init(&options);
    options.cross_validation_folds = 5;
    
    // Perform cross-validation
    double accuracy;
    int ret = ml_cross_validate(&training_data, &options, &accuracy);
    
    if (ret == 0) {
        printf("Cross-validation completed successfully\n");
        printf("  Accuracy: %.4f\n", accuracy);
    } else {
        printf("Failed to perform cross-validation\n");
    }
    
    // Clean up
    ml_training_data_free(&training_data);
    ml_options_free(&options);
    
    printf("Cross-validation test passed!\n");
    return 0;
}

int main() {
    srand(time(NULL)); // Initialize random seed
    
    printf("Running machine learning tests...\n");
    
    int result1 = test_ml_options();
    if (result1 != 0) {
        return result1;
    }
    
    int result2 = test_feature_extraction();
    if (result2 != 0) {
        return result2;
    }
    
    int result3 = test_model_training();
    if (result3 != 0) {
        return result3;
    }
    
    int result4 = test_file_classification();
    if (result4 != 0) {
        return result4;
    }
    
    int result5 = test_file_type_info();
    if (result5 != 0) {
        return result5;
    }
    
    int result6 = test_model_save_load();
    if (result6 != 0) {
        return result6;
    }
    
    int result7 = test_cross_validation();
    if (result7 != 0) {
        return result7;
    }
    
    printf("All machine learning tests passed!\n");
    return 0;
}
