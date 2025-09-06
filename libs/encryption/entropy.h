#ifndef LIBS_ENCRYPTION_ENTROPY_H
#define LIBS_ENCRYPTION_ENTROPY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Calculate entropy of data
double encryption_calculate_entropy_internal(const uint8_t* data, size_t size);

#ifdef __cplusplus
}
#endif

#endif // LIBS_ENCRYPTION_ENTROPY_H