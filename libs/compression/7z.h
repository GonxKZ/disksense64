#ifndef SEVENZIP_H
#define SEVENZIP_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    SEVENZIP_OK,
    SEVENZIP_ERROR
} SevenZip_Result;

SevenZip_Result SevenZip_Decode(const uint8_t *data, size_t size, uint8_t *outputBuffer, size_t outputBufferSize);
SevenZip_Result SevenZip_Encode(const uint8_t *data, size_t size, uint8_t *outputBuffer, size_t outputBufferSize);

#endif // SEVENZIP_H