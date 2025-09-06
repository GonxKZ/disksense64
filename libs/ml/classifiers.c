// Ensure we define _GNU_SOURCE before any system headers
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "ml_internal.h"



// Sigmoid function
static double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

// Softmax function
static void softmax(const double* inputs, double* outputs, size_t size) {
    // Find maximum for numerical stability
    double max_val = inputs[0];
    for (size_t i = 1; i < size; i++) {
        if (inputs[i] > max_val) {
            max_val = inputs[i];
        }
    }
    
    // Compute exponentials
    double sum = 0.0;
    for (size_t i = 0; i < size; i++) {
        outputs[i] = exp(inputs[i] - max_val);
        sum += outputs[i];
    }
    
    // Normalize
    for (size_t i = 0; i < size; i++) {
        outputs[i] /= sum;
    }
}

int ml_logistic_regression_classify(const ml_feature_vector_t* vector, const ml_model_t* model, ml_classification_result_t* result) {
    if (!vector || !model || !result) {
        return -1;
    }
    
    // Compute linear combination
    double sum = model->bias;
    for (size_t i = 0; i < vector->feature_count && i < model->weight_count; i++) {
        sum += vector->features[i] * model->weights[i];
    }
    
    // Apply sigmoid function
    double probability = sigmoid(sum);
    
    // Convert to classification
    result->confidence = probability;
    result->predicted_type = (probability > 0.5) ? FILE_TYPE_MALWARE : FILE_TYPE_UNKNOWN;
    result->explanation = strdup_safe("Logistic regression classification");
    
    return 0;
}

int ml_decision_tree_classify(const ml_feature_vector_t* vector, const ml_model_t* model, ml_classification_result_t* result) {
    if (!vector || !model || !result) {
        return -1;
    }
    
    // Simplified decision tree classification
    // In a real implementation, this would traverse a decision tree
    
    // For now, we'll use a simple rule-based approach
    if (vector->feature_count > 0) {
        double feature_sum = 0.0;
        for (size_t i = 0; i < vector->feature_count; i++) {
            feature_sum += vector->features[i];
        }
        double avg_feature = feature_sum / vector->feature_count;
        
        if (avg_feature > 0.7) {
            result->predicted_type = FILE_TYPE_MALWARE;
            result->confidence = 0.9;
        } else if (avg_feature > 0.3) {
            result->predicted_type = FILE_TYPE_EXECUTABLE;
            result->confidence = 0.7;
        } else {
            result->predicted_type = FILE_TYPE_UNKNOWN;
            result->confidence = 0.5;
        }
    } else {
        result->predicted_type = FILE_TYPE_UNKNOWN;
        result->confidence = 0.0;
    }
    
    result->explanation = strdup_safe("Decision tree classification");
    return 0;
}

int ml_random_forest_classify(const ml_feature_vector_t* vector, const ml_model_t* model, ml_classification_result_t* result) {
    if (!vector || !model || !result) {
        return -1;
    }
    
    // Simplified random forest classification
    // In a real implementation, this would combine multiple decision trees
    
    // For now, we'll average several simple classifications
    file_type_t votes[5];
    double confidences[5];
    
    // Simulate 5 "trees" making predictions
    for (int i = 0; i < 5; i++) {
        double feature_sum = 0.0;
        for (size_t j = 0; j < vector->feature_count; j++) {
            // Add some randomness to each "tree"
            feature_sum += vector->features[j] + ((double)rand() / RAND_MAX - 0.5) * 0.2;
        }
        
        double avg_feature = feature_sum / vector->feature_count;
        
        if (avg_feature > 0.7) {
            votes[i] = FILE_TYPE_MALWARE;
            confidences[i] = 0.9;
        } else if (avg_feature > 0.3) {
            votes[i] = FILE_TYPE_EXECUTABLE;
            confidences[i] = 0.7;
        } else {
            votes[i] = FILE_TYPE_UNKNOWN;
            confidences[i] = 0.5;
        }
    }
    
    // Majority vote
    int malware_votes = 0, executable_votes = 0, unknown_votes = 0;
    double total_confidence = 0.0;
    
    for (int i = 0; i < 5; i++) {
        total_confidence += confidences[i];
        switch (votes[i]) {
            case FILE_TYPE_MALWARE:
                malware_votes++;
                break;
            case FILE_TYPE_EXECUTABLE:
                executable_votes++;
                break;
            default:
                unknown_votes++;
                break;
        }
    }
    
    // Determine winner
    if (malware_votes >= executable_votes && malware_votes >= unknown_votes) {
        result->predicted_type = FILE_TYPE_MALWARE;
    } else if (executable_votes >= unknown_votes) {
        result->predicted_type = FILE_TYPE_EXECUTABLE;
    } else {
        result->predicted_type = FILE_TYPE_UNKNOWN;
    }
    
    result->confidence = total_confidence / 5.0;
    result->explanation = strdup_safe("Random forest classification");
    return 0;
}

int ml_svm_classify(const ml_feature_vector_t* vector, const ml_model_t* model, ml_classification_result_t* result) {
    if (!vector || !model || !result) {
        return -1;
    }
    
    // Simplified SVM classification
    // In a real implementation, this would use support vectors
    
    // Compute decision function
    double sum = model->bias;
    for (size_t i = 0; i < vector->feature_count && i < model->weight_count; i++) {
        sum += vector->features[i] * model->weights[i];
    }
    
    // Apply sign function for binary classification
    result->predicted_type = (sum > 0) ? FILE_TYPE_MALWARE : FILE_TYPE_UNKNOWN;
    result->confidence = fabs(sum) / (fabs(sum) + 1.0); // Normalize confidence
    result->explanation = strdup_safe("Support vector machine classification");
    
    return 0;
}

int ml_neural_network_classify(const ml_feature_vector_t* vector, const ml_model_t* model, ml_classification_result_t* result) {
    if (!vector || !model || !result) {
        return -1;
    }
    
    // Simplified neural network classification
    // In a real implementation, this would perform forward propagation
    
    // For now, we'll use a simple feedforward network with one hidden layer
    const int hidden_units = 10;
    double hidden_layer[hidden_units];
    double output_layer[FILE_TYPE_MALWARE + 1]; // All file types
    
    // Initialize output layer
    for (int i = 0; i <= FILE_TYPE_MALWARE; i++) {
        output_layer[i] = 0.0;
    }
    
    // Compute hidden layer (simplified)
    for (int i = 0; i < hidden_units; i++) {
        double sum = 0.0;
        for (size_t j = 0; j < vector->feature_count && j < model->weight_count; j++) {
            // Simplified weight application
            sum += vector->features[j] * model->weights[j % model->weight_count];
        }
        hidden_layer[i] = sigmoid(sum);
    }
    
    // Compute output layer (simplified)
    for (int i = 0; i <= FILE_TYPE_MALWARE; i++) {
        double sum = 0.0;
        for (int j = 0; j < hidden_units; j++) {
            sum += hidden_layer[j] * ((double)rand() / RAND_MAX);
        }
        output_layer[i] = sum;
    }
    
    // Apply softmax to get probabilities
    double probabilities[FILE_TYPE_MALWARE + 1];
    softmax(output_layer, probabilities, FILE_TYPE_MALWARE + 1);
    
    // Find class with highest probability
    double max_prob = probabilities[0];
    int best_class = 0;
    
    for (int i = 1; i <= FILE_TYPE_MALWARE; i++) {
        if (probabilities[i] > max_prob) {
            max_prob = probabilities[i];
            best_class = i;
        }
    }
    
    result->predicted_type = (file_type_t)best_class;
    result->confidence = max_prob;
    result->explanation = strdup_safe("Neural network classification");
    
    return 0;
}