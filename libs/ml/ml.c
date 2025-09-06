#include "ml_internal.h"

char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

void ml_options_init(ml_options_t* options) {
    if (options) {
        memset(options, 0, sizeof(ml_options_t));
        options->use_metadata = 1;
        options->use_hash_features = 1;
        options->use_content_features = 1;
        options->use_fuzzy_hashing = 1;
        options->max_features = 100;
        options->learning_rate = 0.01;
        options->max_iterations = 1000;
        options->cross_validation_folds = 5;
    }
}

void ml_options_free(ml_options_t* options) {
    if (options) {
        memset(options, 0, sizeof(ml_options_t));
    }
}

void ml_feature_vector_init(ml_feature_vector_t* vector) {
    if (vector) {
        memset(vector, 0, sizeof(ml_feature_vector_t));
    }
}

void ml_feature_vector_free(ml_feature_vector_t* vector) {
    if (vector) {
        free(vector->features);
        free(vector->file_path);
        memset(vector, 0, sizeof(ml_feature_vector_t));
    }
}

void ml_training_data_init(ml_training_data_t* data) {
    if (data) {
        memset(data, 0, sizeof(ml_training_data_t));
    }
}

int ml_training_data_add_vector(ml_training_data_t* data, const ml_feature_vector_t* vector) {
    if (!data || !vector) {
        return -1;
    }
    
    // Reallocate if needed
    if (data->vector_count >= data->capacity) {
        size_t new_capacity = (data->capacity == 0) ? 16 : data->capacity * 2;
        ml_feature_vector_t* new_vectors = (ml_feature_vector_t*)realloc(data->vectors, new_capacity * sizeof(ml_feature_vector_t));
        if (!new_vectors) {
            return -1;
        }
        data->vectors = new_vectors;
        data->capacity = new_capacity;
    }
    
    // Copy vector data
    ml_feature_vector_t* new_vector = &data->vectors[data->vector_count];
    ml_feature_vector_init(new_vector);
    
    new_vector->feature_count = vector->feature_count;
    new_vector->file_path = strdup_safe(vector->file_path);
    new_vector->true_label = vector->true_label;
    new_vector->predicted_label = vector->predicted_label;
    new_vector->confidence = vector->confidence;
    
    if (vector->feature_count > 0) {
        new_vector->features = (double*)malloc(vector->feature_count * sizeof(double));
        if (new_vector->features) {
            memcpy(new_vector->features, vector->features, vector->feature_count * sizeof(double));
        }
    }
    
    if ((!new_vector->features && vector->feature_count > 0) || 
        (!new_vector->file_path && vector->file_path)) {
        // Clean up on failure
        ml_feature_vector_free(new_vector);
        return -1;
    }
    
    data->vector_count++;
    return 0;
}

void ml_training_data_free(ml_training_data_t* data) {
    if (data) {
        if (data->vectors) {
            for (size_t i = 0; i < data->vector_count; i++) {
                ml_feature_vector_free(&data->vectors[i]);
            }
            free(data->vectors);
        }
        memset(data, 0, sizeof(ml_training_data_t));
    }
}

void ml_model_init(ml_model_t* model) {
    if (model) {
        memset(model, 0, sizeof(ml_model_t));
    }
}

void ml_model_free(ml_model_t* model) {
    if (model) {
        free(model->model_name);
        free(model->weights);
        memset(model, 0, sizeof(ml_model_t));
    }
}

void ml_classification_result_init(ml_classification_result_t* result) {
    if (result) {
        memset(result, 0, sizeof(ml_classification_result_t));
    }
}

void ml_classification_result_free(ml_classification_result_t* result) {
    if (result) {
        free(result->explanation);
        memset(result, 0, sizeof(ml_classification_result_t));
    }
}

int ml_extract_features(const char* file_path, const ml_options_t* options, ml_feature_vector_t* vector) {
    if (!file_path || !vector) {
        return -1;
    }
    
    ml_feature_vector_init(vector);
    
    // Set file path
    vector->file_path = strdup_safe(file_path);
    
    // Determine number of features based on options
    size_t feature_count = 0;
    
    if (options->use_metadata) {
        feature_count += 10; // File size, timestamps, permissions, etc.
    }
    
    if (options->use_hash_features) {
        feature_count += 5; // Hash-based features
    }
    
    if (options->use_content_features) {
        feature_count += 20; // Content-based features
    }
    
    if (options->use_fuzzy_hashing) {
        feature_count += 3; // Fuzzy hashing features
    }
    
    // Limit to max_features
    if (feature_count > options->max_features) {
        feature_count = options->max_features;
    }
    
    vector->feature_count = feature_count;
    
    // Allocate features array
    vector->features = (double*)calloc(feature_count, sizeof(double));
    if (!vector->features) {
        ml_feature_vector_free(vector);
        return -1;
    }
    
    // Extract features based on options
    size_t feature_index = 0;
    
    if (options->use_metadata && feature_index < feature_count) {
        // Extract metadata features
        metadata_info_t info;
        if (metadata_get_file_info(file_path, &info) == 0) {
            // File size (normalized)
            if (feature_index < feature_count) {
                vector->features[feature_index++] = log(info.size + 1) / 20.0; // Log normalization
            }
            
            // Timestamps (days since epoch)
            if (feature_index < feature_count) {
                vector->features[feature_index++] = (double)info.last_write_time / (24.0 * 3600.0 * 365.0 * 50.0);
            }
            
            // Permissions
            if (feature_index < feature_count) {
                vector->features[feature_index++] = (double)info.permissions / 0x1FF; // Normalize to 0-1
            }
            
            // File type flags
            if (feature_index < feature_count) {
                vector->features[feature_index++] = info.is_directory ? 1.0 : 0.0;
            }
            
            if (feature_index < feature_count) {
                vector->features[feature_index++] = info.is_symlink ? 1.0 : 0.0;
            }
            
            if (feature_index < feature_count) {
                vector->features[feature_index++] = info.is_hidden ? 1.0 : 0.0;
            }
            
            // Cleanup
            metadata_free_info(&info);
        }
    }
    
    if (options->use_hash_features && feature_index < feature_count) {
        // Extract hash-based features
        // This would compute various hashes and extract features from them
        // For now, we'll use dummy values
        for (int i = 0; i < 5 && feature_index < feature_count; i++) {
            vector->features[feature_index++] = (double)rand() / RAND_MAX;
        }
    }
    
    if (options->use_content_features && feature_index < feature_count) {
        // Extract content-based features
        // This would analyze file content for features like byte frequency, entropy, etc.
        // For now, we'll use dummy values
        for (int i = 0; i < 20 && feature_index < feature_count; i++) {
            vector->features[feature_index++] = (double)rand() / RAND_MAX;
        }
    }
    
    if (options->use_fuzzy_hashing && feature_index < feature_count) {
        // Extract fuzzy hashing features
        fuzzy_hash_result_t fuzzy_result;
        if (fuzzy_hash_file(file_path, FUZZY_HASH_SSDEEP, &fuzzy_result) == 0) {
            // Use hash length and other properties as features
            if (feature_index < feature_count) {
                vector->features[feature_index++] = (double)fuzzy_result.hash_length / 1000.0;
            }
            
            // Cleanup
            fuzzy_hash_free(&fuzzy_result);
        }
        
        // Add more fuzzy hash features
        if (feature_index < feature_count) {
            vector->features[feature_index++] = (double)rand() / RAND_MAX;
        }
        
        if (feature_index < feature_count) {
            vector->features[feature_index++] = (double)rand() / RAND_MAX;
        }
    }
    
    return 0;
}

int ml_train_model(const ml_training_data_t* training_data, const ml_options_t* options, ml_model_t* model) {
    if (!training_data || !model) {
        return -1;
    }
    
    ml_model_init(model);
    
    // In a real implementation, this would train a machine learning model
    // For now, we'll create a dummy model
    
    model->model_name = strdup_safe("LogisticRegression");
    model->model_type = 1; // Logistic regression
    model->weight_count = training_data->vectors[0].feature_count;
    model->weights = (double*)malloc(model->weight_count * sizeof(double));
    model->bias = 0.0;
    model->is_trained = 1;
    model->accuracy = 0.85; // Dummy accuracy
    
    // Initialize weights randomly
    for (size_t i = 0; i < model->weight_count; i++) {
        model->weights[i] = ((double)rand() / RAND_MAX) * 2.0 - 1.0; // Random value between -1 and 1
    }
    
    return 0;
}

int ml_classify_file(const char* file_path, const ml_model_t* model, const ml_options_t* options, ml_classification_result_t* result) {
    if (!file_path || !model || !result) {
        return -1;
    }
    
    ml_classification_result_init(result);
    
    // Extract features from file
    ml_feature_vector_t vector;
    if (ml_extract_features(file_path, options, &vector) != 0) {
        return -1;
    }
    
    // Classify the feature vector
    int ret = ml_classify_vector(&vector, model, result);
    
    // Clean up
    ml_feature_vector_free(&vector);
    
    return ret;
}

int ml_classify_vector(const ml_feature_vector_t* vector, const ml_model_t* model, ml_classification_result_t* result) {
    if (!vector || !model || !result) {
        return -1;
    }
    
    ml_classification_result_init(result);
    
    // In a real implementation, this would use the trained model to classify the vector
    // For now, we'll use a simple heuristic-based approach
    
    if (!vector->features || vector->feature_count == 0) {
        result->predicted_type = FILE_TYPE_UNKNOWN;
        result->confidence = 0.0;
        result->explanation = strdup_safe("No features available");
        return 0;
    }
    
    // Simple classification based on feature values
    double sum = model->bias;
    for (size_t i = 0; i < vector->feature_count && i < model->weight_count; i++) {
        sum += vector->features[i] * model->weights[i];
    }
    
    // Apply sigmoid function for probability
    double probability = 1.0 / (1.0 + exp(-sum));
    
    // Convert probability to file type (simplified)
    if (probability > 0.9) {
        result->predicted_type = FILE_TYPE_MALWARE;
        result->confidence = probability;
        result->explanation = strdup_safe("High confidence malware detection");
    } else if (probability > 0.7) {
        result->predicted_type = FILE_TYPE_EXECUTABLE;
        result->confidence = probability;
        result->explanation = strdup_safe("Likely executable file");
    } else if (vector->features[0] > 0.5) { // Size feature
        result->predicted_type = FILE_TYPE_DOCUMENT;
        result->confidence = 0.7;
        result->explanation = strdup_safe("Large file, likely document");
    } else {
        result->predicted_type = FILE_TYPE_UNKNOWN;
        result->confidence = 0.5;
        result->explanation = strdup_safe("Uncertain classification");
    }
    
    return 0;
}

int ml_evaluate_model(const ml_model_t* model, const ml_training_data_t* test_data, double* accuracy) {
    if (!model || !test_data || !accuracy) {
        return -1;
    }
    
    // In a real implementation, this would evaluate the model on test data
    // For now, we'll return the model's stored accuracy
    *accuracy = model->accuracy;
    
    return 0;
}

int ml_save_model(const ml_model_t* model, const char* file_path) {
    if (!model || !file_path) {
        return -1;
    }
    
    FILE* file = fopen(file_path, "wb");
    if (!file) {
        return -1;
    }
    
    // Write model data
    // In a real implementation, this would serialize the model
    // For now, we'll write dummy data
    
    fprintf(file, "ML Model: %s\n", model->model_name ? model->model_name : "Unknown");
    fprintf(file, "Type: %d\n", model->model_type);
    fprintf(file, "Features: %lu\n", (unsigned long)model->weight_count);
    fprintf(file, "Accuracy: %.4f\n", model->accuracy);
    fprintf(file, "Trained: %s\n", model->is_trained ? "Yes" : "No");
    
    fclose(file);
    return 0;
}

int ml_load_model(const char* file_path, ml_model_t* model) {
    if (!file_path || !model) {
        return -1;
    }
    
    ml_model_init(model);
    
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return -1;
    }
    
    // In a real implementation, this would deserialize the model
    // For now, we'll create a dummy model
    
    model->model_name = strdup_safe("LoadedModel");
    model->model_type = 1;
    model->weight_count = 10;
    model->weights = (double*)malloc(model->weight_count * sizeof(double));
    model->bias = 0.0;
    model->is_trained = 1;
    model->accuracy = 0.8;
    
    // Initialize weights
    for (size_t i = 0; i < model->weight_count; i++) {
        model->weights[i] = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
    }
    
    fclose(file);
    return 0;
}

const char* ml_get_file_type_name(file_type_t type) {
    switch (type) {
        case FILE_TYPE_UNKNOWN:
            return "Unknown";
        case FILE_TYPE_EXECUTABLE:
            return "Executable";
        case FILE_TYPE_DOCUMENT:
            return "Document";
        case FILE_TYPE_IMAGE:
            return "Image";
        case FILE_TYPE_AUDIO:
            return "Audio";
        case FILE_TYPE_VIDEO:
            return "Video";
        case FILE_TYPE_ARCHIVE:
            return "Archive";
        case FILE_TYPE_DATABASE:
            return "Database";
        case FILE_TYPE_LOG:
            return "Log";
        case FILE_TYPE_CONFIG:
            return "Configuration";
        case FILE_TYPE_TEMP:
            return "Temporary";
        case FILE_TYPE_SYSTEM:
            return "System";
        case FILE_TYPE_MALWARE:
            return "Malware";
        default:
            return "Unknown";
    }
}

const char* ml_get_file_type_description(file_type_t type) {
    switch (type) {
        case FILE_TYPE_UNKNOWN:
            return "Unknown file type";
        case FILE_TYPE_EXECUTABLE:
            return "Executable program or script";
        case FILE_TYPE_DOCUMENT:
            return "Text document or office document";
        case FILE_TYPE_IMAGE:
            return "Image file (JPEG, PNG, etc.)";
        case FILE_TYPE_AUDIO:
            return "Audio file (MP3, WAV, etc.)";
        case FILE_TYPE_VIDEO:
            return "Video file (MP4, AVI, etc.)";
        case FILE_TYPE_ARCHIVE:
            return "Compressed archive (ZIP, RAR, etc.)";
        case FILE_TYPE_DATABASE:
            return "Database file";
        case FILE_TYPE_LOG:
            return "Log file";
        case FILE_TYPE_CONFIG:
            return "Configuration file";
        case FILE_TYPE_TEMP:
            return "Temporary file";
        case FILE_TYPE_SYSTEM:
            return "System file";
        case FILE_TYPE_MALWARE:
            return "Malicious software";
        default:
            return "Unknown file type";
    }
}

int ml_generate_training_data(const char* directory_path, const ml_options_t* options, ml_training_data_t* training_data) {
    if (!directory_path || !training_data) {
        return -1;
    }
    
    ml_training_data_init(training_data);
    
    // Open directory
    DIR* dir = opendir(directory_path);
    if (!dir) {
        return -1;
    }
    
    // Process directory entries
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip current and parent directory
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Build full path
        size_t path_len = strlen(directory_path);
        size_t name_len = strlen(entry->d_name);
        char* full_path = (char*)malloc(path_len + name_len + 2);
        if (!full_path) {
            continue;
        }
        
        sprintf(full_path, "%s/%s", directory_path, entry->d_name);
        
        // Check if it's a regular file
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            // Extract features
            ml_feature_vector_t vector;
            if (ml_extract_features(full_path, options, &vector) == 0) {
                // Set true label based on file extension or other heuristics
                const char* ext = strrchr(entry->d_name, '.');
                if (ext) {
                    if (strcasecmp(ext, ".exe") == 0 || strcasecmp(ext, ".dll") == 0) {
                        vector.true_label = FILE_TYPE_EXECUTABLE;
                    } else if (strcasecmp(ext, ".txt") == 0 || strcasecmp(ext, ".doc") == 0 || 
                               strcasecmp(ext, ".pdf") == 0) {
                        vector.true_label = FILE_TYPE_DOCUMENT;
                    } else if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".png") == 0 || 
                               strcasecmp(ext, ".gif") == 0) {
                        vector.true_label = FILE_TYPE_IMAGE;
                    } else if (strcasecmp(ext, ".mp3") == 0 || strcasecmp(ext, ".wav") == 0) {
                        vector.true_label = FILE_TYPE_AUDIO;
                    } else if (strcasecmp(ext, ".mp4") == 0 || strcasecmp(ext, ".avi") == 0) {
                        vector.true_label = FILE_TYPE_VIDEO;
                    } else if (strcasecmp(ext, ".zip") == 0 || strcasecmp(ext, ".rar") == 0) {
                        vector.true_label = FILE_TYPE_ARCHIVE;
                    } else {
                        vector.true_label = FILE_TYPE_UNKNOWN;
                    }
                } else {
                    vector.true_label = FILE_TYPE_UNKNOWN;
                }
                
                // Add to training data
                ml_training_data_add_vector(training_data, &vector);
                ml_feature_vector_free(&vector);
            }
        }
        
        free(full_path);
    }
    
    closedir(dir);
    return 0;
}