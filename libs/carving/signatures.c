#include "signatures.h"

// JPEG signatures
static const uint8_t jpeg_header[] = {0xFF, 0xD8, 0xFF};
static const uint8_t jpeg_footer[] = {0xFF, 0xD9};

// PNG signatures
static const uint8_t png_header[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
static const uint8_t png_footer[] = {0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82};

// GIF signatures
static const uint8_t gif_header1[] = {0x47, 0x49, 0x46, 0x38, 0x37, 0x61}; // GIF87a
static const uint8_t gif_header2[] = {0x47, 0x49, 0x46, 0x38, 0x39, 0x61}; // GIF89a

// PDF signatures
static const uint8_t pdf_header[] = {0x25, 0x50, 0x44, 0x46}; // %PDF
static const uint8_t pdf_footer[] = {0x25, 0x25, 0x45, 0x4F, 0x46}; // %%EOF

// ZIP signatures
static const uint8_t zip_header[] = {0x50, 0x4B, 0x03, 0x04};
static const uint8_t zip_footer[] = {0x50, 0x4B, 0x05, 0x06}; // End of central directory

// RAR signatures
static const uint8_t rar_header1[] = {0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x00}; // RAR v1.5
static const uint8_t rar_header2[] = {0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x01, 0x00}; // RAR v5.0

// MP3 signatures
static const uint8_t mp3_header1[] = {0x49, 0x44, 0x33}; // ID3
static const uint8_t mp3_header2[] = {0xFF, 0xFB}; // MPEG-1 Layer 3
static const uint8_t mp3_header3[] = {0xFF, 0xF3}; // MPEG-1 Layer 3
static const uint8_t mp3_header4[] = {0xFF, 0xF2}; // MPEG-1 Layer 3

// AVI signatures
static const uint8_t avi_header[] = {0x52, 0x49, 0x46, 0x46}; // RIFF

// BMP signatures
static const uint8_t bmp_header[] = {0x42, 0x4D}; // BM

// File signatures database
const file_signature_t g_file_signatures[] = {
    // JPEG
    {
        "jpg",
        jpeg_header, sizeof(jpeg_header),
        jpeg_footer, sizeof(jpeg_footer),
        100, 100 * 1024 * 1024 // 100MB max
    },
    
    // PNG
    {
        "png",
        png_header, sizeof(png_header),
        png_footer, sizeof(png_footer),
        100, 100 * 1024 * 1024 // 100MB max
    },
    
    // GIF
    {
        "gif",
        gif_header1, sizeof(gif_header1),
        NULL, 0,
        100, 50 * 1024 * 1024 // 50MB max
    },
    
    {
        "gif",
        gif_header2, sizeof(gif_header2),
        NULL, 0,
        100, 50 * 1024 * 1024 // 50MB max
    },
    
    // PDF
    {
        "pdf",
        pdf_header, sizeof(pdf_header),
        pdf_footer, sizeof(pdf_footer),
        100, 100 * 1024 * 1024 // 100MB max
    },
    
    // ZIP
    {
        "zip",
        zip_header, sizeof(zip_header),
        zip_footer, sizeof(zip_footer),
        100, 100 * 1024 * 1024 // 100MB max
    },
    
    // RAR
    {
        "rar",
        rar_header1, sizeof(rar_header1),
        NULL, 0,
        100, 100 * 1024 * 1024 // 100MB max
    },
    
    {
        "rar",
        rar_header2, sizeof(rar_header2),
        NULL, 0,
        100, 100 * 1024 * 1024 // 100MB max
    },
    
    // MP3
    {
        "mp3",
        mp3_header1, sizeof(mp3_header1),
        NULL, 0,
        100, 100 * 1024 * 1024 // 100MB max
    },
    
    {
        "mp3",
        mp3_header2, sizeof(mp3_header2),
        NULL, 0,
        100, 100 * 1024 * 1024 // 100MB max
    },
    
    {
        "mp3",
        mp3_header3, sizeof(mp3_header3),
        NULL, 0,
        100, 100 * 1024 * 1024 // 100MB max
    },
    
    {
        "mp3",
        mp3_header4, sizeof(mp3_header4),
        NULL, 0,
        100, 100 * 1024 * 1024 // 100MB max
    },
    
    // AVI
    {
        "avi",
        avi_header, sizeof(avi_header),
        NULL, 0,
        100, 200 * 1024 * 1024 // 200MB max
    },
    
    // BMP
    {
        "bmp",
        bmp_header, sizeof(bmp_header),
        NULL, 0,
        100, 100 * 1024 * 1024 // 100MB max
    }
};

const size_t g_file_signatures_count = sizeof(g_file_signatures) / sizeof(file_signature_t);

// Function to get signatures
const file_signature_t* carving_get_signatures(size_t* count) {
    if (count) {
        *count = g_file_signatures_count;
    }
    return g_file_signatures;
}