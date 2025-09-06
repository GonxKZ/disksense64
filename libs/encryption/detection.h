#ifndef LIBS_ENCRYPTION_DETECTION_H
#define LIBS_ENCRYPTION_DETECTION_H

#include "encryption.h"

#ifdef __cplusplus
extern "C" {
#endif

// Detect cipher type from data
int encryption_detect_cipher_internal(const uint8_t* data, size_t size, char** cipher_name, double* confidence);

// Detect encryption algorithm from data
int encryption_detect_algorithm_internal(const uint8_t* data, size_t size, char** algorithm_name, double* confidence);

#ifdef __cplusplus
}
#endif

#endif // LIBS_ENCRYPTION_DETECTION_H