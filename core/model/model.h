#ifndef DISKSENSE64_MODEL_H
#define DISKSENSE64_MODEL_H

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

// Type definitions for our core data model
using VolumeId = uint64_t;
using FileId = uint64_t;
using PathId = uint64_t;

// File attributes structure
struct FileAttributes {
    bool readOnly : 1;
    bool hidden : 1;
    bool system : 1;
    bool directory : 1;
    bool archive : 1;
    bool temporary : 1;
    bool sparse : 1;
    bool reparsePoint : 1;
    bool compressed : 1;
    bool encrypted : 1;
    bool offline : 1;
    bool notContentIndexed : 1;
    bool virtualFile : 1;
    
    FileAttributes() : readOnly(false), hidden(false), system(false), directory(false),
                       archive(false), temporary(false), sparse(false), reparsePoint(false),
                       compressed(false), encrypted(false), offline(false),
                       notContentIndexed(false), virtualFile(false) {}
};

// Timestamps structure
struct FileTimestamps {
    uint64_t creationTime;     // FILETIME as 100-nanosecond intervals since January 1, 1601 UTC
    uint64_t lastWriteTime;
    uint64_t lastAccessTime;
    uint64_t changeTime;
    
    FileTimestamps() : creationTime(0), lastWriteTime(0), lastAccessTime(0), changeTime(0) {}
};

// File entry structure
struct FileEntry {
    VolumeId volumeId;
    FileId fileId;
    PathId pathId;
    uint64_t sizeLogical;      // Actual file size in bytes
    uint64_t sizeOnDisk;       // Size on disk (cluster-aligned)
    FileAttributes attributes;
    FileTimestamps timestamps;
    
    // Signatures
    std::optional<std::vector<uint8_t>> headTail16;  // 16KB head + 16KB tail hash
    std::optional<std::vector<uint8_t>> sha256;      // Full file SHA-256 hash
    std::optional<std::vector<uint8_t>> perceptualHash; // Image/audio perceptual hash
    
    // Media information
    std::optional<std::pair<uint32_t, uint32_t>> imageDimensions; // width x height
    std::optional<uint64_t> audioDuration; // Duration in milliseconds
    
    FileEntry() : volumeId(0), fileId(0), pathId(0), sizeLogical(0), sizeOnDisk(0) {}
};

// Chunk information for content-defined chunking
struct FileChunk {
    uint64_t offset;
    uint64_t length;
    std::vector<uint8_t> hash; // BLAKE3/SHA-256 hash of the chunk
    
    FileChunk(uint64_t off, uint64_t len) : offset(off), length(len) {}
};

#endif // DISKSENSE64_MODEL_H