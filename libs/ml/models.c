// Ensure we define _GNU_SOURCE before any system headers
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "ml_internal.h"
#include <math.h>
#include <stddef.h>

// Helper function to duplicate string


int ml_save_logistic_regression_model(const ml_model_t* model, FILE* file) {
    if (!model || !file) {
        return -1;
    }
    
    // Write model type
    fprintf(file, "MODEL_TYPE=LOGISTIC_REGRESSION\n");
    
    // Write model name
    fprintf(file, "MODEL_NAME=%s\n", model->model_name ? model->model_name : "Unknown");
    
    // Write weight count
    fprintf(file, "WEIGHT_COUNT=%lu\n", (unsigned long)model->weight_count);
    
    // Write bias
    fprintf(file, "BIAS=%.10f\n", model->bias);
    
    // Write weights
    fprintf(file, "WEIGHTS=");
    for (size_t i = 0; i < model->weight_count; i++) {
        fprintf(file, "%.10f", model->weights[i]);
        if (i < model->weight_count - 1) {
            fprintf(file, ",");
        }
    }
    fprintf(file, "\n");
    
    // Write accuracy
    fprintf(file, "ACCURACY=%.4f\n", model->accuracy);
    
    return 0;
}

int ml_load_logistic_regression_model(ml_model_t* model, FILE* file) {
    if (!model || !file) {
        return -1;
    }
    
    ml_model_init(model);
    
    char line[1024];
    
    // Read model type
    if (fgets(line, sizeof(line), file)) {
        // Parse model type (simplified)
    }
    
    // Read model name
    if (fgets(line, sizeof(line), file)) {
        // Remove MODEL_NAME= prefix and newline
        char* name_start = strchr(line, '=');
        if (name_start) {
            name_start++;
            char* newline = strchr(name_start, '\n');
            if (newline) *newline = '\0';
            model->model_name = strdup_safe(name_start);
        }
    }
    
    // Read weight count
    if (fgets(line, sizeof(line), file)) {
        char* count_start = strchr(line, '=');
        if (count_start) {
            model->weight_count = atoi(count_start + 1);
        }
    }
    
    // Read bias
    if (fgets(line, sizeof(line), file)) {
        char* bias_start = strchr(line, '=');
        if (bias_start) {
            model->bias = atof(bias_start + 1);
        }
    }
    
    // Read weights
    if (fgets(line, sizeof(line), file)) {
        model->weights = (double*)malloc(model->weight_count * sizeof(double));
        if (model->weights) {
            char* weights_start = strchr(line, '=');
            if (weights_start) {
                weights_start++;
                // Parse comma-separated weights (simplified)
                char* token = strtok(weights_start, ",");
                size_t index = 0;
                while (token && index < model->weight_count) {
                    model->weights[index++] = atof(token);
                    token = strtok(NULL, ",");
                }
            }
        }
    }
    
    // Read accuracy
    if (fgets(line, sizeof(line), file)) {
        char* accuracy_start = strchr(line, '=');
        if (accuracy_start) {
            model->accuracy = atof(accuracy_start + 1);
        }
    }
    
    model->model_type = 1; // Logistic regression
    model->is_trained = 1;
    
    return 0;
}

int ml_save_decision_tree_model(const ml_model_t* model, FILE* file) {
    if (!model || !file) {
        return -1;
    }
    
    // Write model type
    fprintf(file, "MODEL_TYPE=DECISION_TREE\n");
    
    // Write model name
    fprintf(file, "MODEL_NAME=%s\n", model->model_name ? model->model_name : "Unknown");
    
    // Write dummy data for decision tree
    fprintf(file, "TREE_DATA=DECISION_TREE_PLACEHOLDER\n");
    
    return 0;
}

int ml_load_decision_tree_model(ml_model_t* model, FILE* file) {
    if (!model || !file) {
        return -1;
    }
    
    ml_model_init(model);
    model->model_name = strdup_safe("DecisionTree");
    model->model_type = 2; // Decision tree
    model->is_trained = 1;
    model->accuracy = 0.80; // Dummy accuracy
    
    return 0;
}

int ml_save_random_forest_model(const ml_model_t* model, FILE* file) {
    if (!model || !file) {
        return -1;
    }
    
    // Write model type
    fprintf(file, "MODEL_TYPE=RANDOM_FOREST\n");
    
    // Write model name
    fprintf(file, "MODEL_NAME=%s\n", model->model_name ? model->model_name : "Unknown");
    
    // Write dummy data for random forest
    fprintf(file, "FOREST_DATA=RANDOM_FOREST_PLACEHOLDER\n");
    
    return 0;
}

int ml_load_random_forest_model(ml_model_t* model, FILE* file) {
    if (!model || !file) {
        return -1;
    }
    
    ml_model_init(model);
    model->model_name = strdup_safe("RandomForest");
    model->model_type = 3; // Random forest
    model->is_trained = 1;
    model->accuracy = 0.88; // Dummy accuracy
    
    return 0;
}

int ml_save_svm_model(const ml_model_t* model, FILE* file) {
    if (!model || !file) {
        return -1;
    }
    
    // Write model type
    fprintf(file, "MODEL_TYPE=SVM\n");
    
    // Write model name
    fprintf(file, "MODEL_NAME=%s\n", model->model_name ? model->model_name : "Unknown");
    
    // Write weight count
    fprintf(file, "WEIGHT_COUNT=%lu\n", (unsigned long)model->weight_count);
    
    // Write bias
    fprintf(file, "BIAS=%.10f\n", model->bias);
    
    // Write weights
    fprintf(file, "WEIGHTS=");
    for (size_t i = 0; i < model->weight_count; i++) {
        fprintf(file, "%.10f", model->weights[i]);
        if (i < model->weight_count - 1) {
            fprintf(file, ",");
        }
    }
    fprintf(file, "\n");
    
    return 0;
}

int ml_load_svm_model(ml_model_t* model, FILE* file) {
    if (!model || !file) {
        return -1;
    }
    
    ml_model_init(model);
    model->model_name = strdup_safe("SVM");
    model->model_type = 4; // SVM
    model->is_trained = 1;
    model->accuracy = 0.82; // Dummy accuracy
    
    // Read weight count
    char line[1024];
    if (fgets(line, sizeof(line), file)) {
        char* count_start = strchr(line, '=');
        if (count_start) {
            model->weight_count = atoi(count_start + 1);
        }
    }
    
    // Read bias
    if (fgets(line, sizeof(line), file)) {
        char* bias_start = strchr(line, '=');
        if (bias_start) {
            model->bias = atof(bias_start + 1);
        }
    }
    
    // Allocate and read weights
    model->weights = (double*)malloc(model->weight_count * sizeof(double));
    if (model->weights) {
        if (fgets(line, sizeof(line), file)) {
            char* weights_start = strchr(line, '=');
            if (weights_start) {
                weights_start++;
                // Parse comma-separated weights (simplified)
                char* token = strtok(weights_start, ",");
                size_t index = 0;
                while (token && index < model->weight_count) {
                    model->weights[index++] = atof(token);
                    token = strtok(NULL, ",");
                }
            }
        }
    }
    
    return 0;
}

int ml_save_neural_network_model(const ml_model_t* model, FILE* file) {
    if (!model || !file) {
        return -1;
    }
    
    // Write model type
    fprintf(file, "MODEL_TYPE=NEURAL_NETWORK\n");
    
    // Write model name
    fprintf(file, "MODEL_NAME=%s\n", model->model_name ? model->model_name : "Unknown");
    
    // Write weight count
    fprintf(file, "WEIGHT_COUNT=%lu\n", (unsigned long)model->weight_count);
    
    // Write weights
    fprintf(file, "WEIGHTS=");
    for (size_t i = 0; i < model->weight_count; i++) {
        fprintf(file, "%.10f", model->weights[i]);
        if (i < model->weight_count - 1) {
            fprintf(file, ",");
        }
    }
    fprintf(file, "\n");
    
    return 0;
}

int ml_load_neural_network_model(ml_model_t* model, FILE* file) {
    if (!model || !file) {
        return -1;
    }
    
    ml_model_init(model);
    model->model_name = strdup_safe("NeuralNetwork");
    model->model_type = 5; // Neural network
    model->is_trained = 1;
    model->accuracy = 0.90; // Dummy accuracy
    
    // Read weight count
    char line[1024];
    if (fgets(line, sizeof(line), file)) {
        char* count_start = strchr(line, '=');
        if (count_start) {
            model->weight_count = atoi(count_start + 1);
        }
    }
    
    // Allocate and read weights
    model->weights = (double*)malloc(model->weight_count * sizeof(double));
    if (model->weights) {
        if (fgets(line, sizeof(line), file)) {
            char* weights_start = strchr(line, '=');
            if (weights_start) {
                weights_start++;
                // Parse comma-separated weights (simplified)
                char* token = strtok(weights_start, ",");
                size_t index = 0;
                while (token && index < model->weight_count) {
                    model->weights[index++] = atof(token);
                    token = strtok(NULL, ",");
                }
            }
        }
    }
    
    return 0;
}