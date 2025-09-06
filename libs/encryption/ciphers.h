#ifndef LIBS_ENCRYPTION_CIPHERS_H
#define LIBS_ENCRYPTION_CIPHERS_H

#include "encryption.h"

#ifdef __cplusplus
extern "C" {
#endif

// Get information about supported ciphers
int encryption_get_cipher_info_internal(cipher_info_t** ciphers, size_t* count);

#ifdef __cplusplus
}
#endif

#endif // LIBS_ENCRYPTION_CIPHERS_H