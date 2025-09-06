#ifndef LIBS_HIDDEN_ROOTKITS_H
#define LIBS_HIDDEN_ROOTKITS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Rootkit information structure
typedef struct {
    const char* name;
    const char* description;
    const char* detection_method;
} rootkit_info_t;

// Get known rootkits database
const rootkit_info_t* hidden_get_known_rootkits(size_t* count);

#ifdef __cplusplus
}
#endif

#endif // LIBS_HIDDEN_ROOTKITS_H