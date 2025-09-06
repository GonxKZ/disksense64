#ifndef LIBS_CARVING_SIGNATURES_H
#define LIBS_CARVING_SIGNATURES_H

#include "carving.h"

#ifdef __cplusplus
extern "C" {
#endif

// File signatures for common file types
extern const file_signature_t g_file_signatures[];
extern const size_t g_file_signatures_count;

#ifdef __cplusplus
}
#endif

#endif // LIBS_CARVING_SIGNATURES_H