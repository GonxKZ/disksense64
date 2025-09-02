#include "model.h"

// FileEntry implementation
FileEntry::FileEntry() 
    : volumeId(0), fileId(0), pathId(0), 
      sizeLogical(0), sizeOnDisk(0) {
}

FileEntry::FileEntry(VolumeId volId, FileId fId, PathId pId, uint64_t size)
    : volumeId(volId), fileId(fId), pathId(pId),
      sizeLogical(size), sizeOnDisk((size + 4095) & ~4095) { // 4KB cluster alignment
}

FileEntry::FileEntry(const FileEntry& other)
    : volumeId(other.volumeId), fileId(other.fileId), pathId(other.pathId),
      sizeLogical(other.sizeLogical), sizeOnDisk(other.sizeOnDisk),
      attributes(other.attributes), timestamps(other.timestamps),
      headTail16(other.headTail16), sha256(other.sha256),
      perceptualHash(other.perceptualHash),
      imageDimensions(other.imageDimensions),
      audioDuration(other.audioDuration) {
}

FileEntry& FileEntry::operator=(const FileEntry& other) {
    if (this != &other) {
        volumeId = other.volumeId;
        fileId = other.fileId;
        pathId = other.pathId;
        sizeLogical = other.sizeLogical;
        sizeOnDisk = other.sizeOnDisk;
        attributes = other.attributes;
        timestamps = other.timestamps;
        headTail16 = other.headTail16;
        sha256 = other.sha256;
        perceptualHash = other.perceptualHash;
        imageDimensions = other.imageDimensions;
        audioDuration = other.audioDuration;
    }
    return *this;
}

FileEntry::FileEntry(FileEntry&& other) noexcept
    : volumeId(other.volumeId), fileId(other.fileId), pathId(other.pathId),
      sizeLogical(other.sizeLogical), sizeOnDisk(other.sizeOnDisk),
      attributes(std::move(other.attributes)), timestamps(std::move(other.timestamps)),
      headTail16(std::move(other.headTail16)), sha256(std::move(other.sha256)),
      perceptualHash(std::move(other.perceptualHash)),
      imageDimensions(std::move(other.imageDimensions)),
      audioDuration(std::move(other.audioDuration)) {
    other.volumeId = 0;
    other.fileId = 0;
    other.pathId = 0;
    other.sizeLogical = 0;
    other.sizeOnDisk = 0;
}

FileEntry& FileEntry::operator=(FileEntry&& other) noexcept {
    if (this != &other) {
        volumeId = other.volumeId;
        fileId = other.fileId;
        pathId = other.pathId;
        sizeLogical = other.sizeLogical;
        sizeOnDisk = other.sizeOnDisk;
        attributes = std::move(other.attributes);
        timestamps = std::move(other.timestamps);
        headTail16 = std::move(other.headTail16);
        sha256 = std::move(other.sha256);
        perceptualHash = std::move(other.perceptualHash);
        imageDimensions = std::move(other.imageDimensions);
        audioDuration = std::move(other.audioDuration);
        
        other.volumeId = 0;
        other.fileId = 0;
        other.pathId = 0;
        other.sizeLogical = 0;
        other.sizeOnDisk = 0;
    }
    return *this;
}

FileEntry::~FileEntry() {
}

bool FileEntry::operator==(const FileEntry& other) const {
    return volumeId == other.volumeId && 
           fileId == other.fileId && 
           pathId == other.pathId &&
           sizeLogical == other.sizeLogical &&
           sizeOnDisk == other.sizeOnDisk;
}

bool FileEntry::operator!=(const FileEntry& other) const {
    return !(*this == other);
}

bool FileEntry::operator<(const FileEntry& other) const {
    if (volumeId != other.volumeId) {
        return volumeId < other.volumeId;
    }
    if (fileId != other.fileId) {
        return fileId < other.fileId;
    }
    return pathId < other.pathId;
}

// FileAttributes implementation
FileAttributes::FileAttributes() 
    : readOnly(false), hidden(false), system(false), directory(false),
      archive(false), temporary(false), sparse(false), reparsePoint(false),
      compressed(false), encrypted(false), offline(false),
      notContentIndexed(false), virtualFile(false) {
}

FileAttributes::FileAttributes(const FileAttributes& other)
    : readOnly(other.readOnly), hidden(other.hidden), system(other.system),
      directory(other.directory), archive(other.archive), temporary(other.temporary),
      sparse(other.sparse), reparsePoint(other.reparsePoint),
      compressed(other.compressed), encrypted(other.encrypted),
      offline(other.offline), notContentIndexed(other.notContentIndexed),
      virtualFile(other.virtualFile) {
}

FileAttributes& FileAttributes::operator=(const FileAttributes& other) {
    if (this != &other) {
        readOnly = other.readOnly;
        hidden = other.hidden;
        system = other.system;
        directory = other.directory;
        archive = other.archive;
        temporary = other.temporary;
        sparse = other.sparse;
        reparsePoint = other.reparsePoint;
        compressed = other.compressed;
        encrypted = other.encrypted;
        offline = other.offline;
        notContentIndexed = other.notContentIndexed;
        virtualFile = other.virtualFile;
    }
    return *this;
}

FileAttributes::FileAttributes(FileAttributes&& other) noexcept
    : readOnly(other.readOnly), hidden(other.hidden), system(other.system),
      directory(other.directory), archive(other.archive), temporary(other.temporary),
      sparse(other.sparse), reparsePoint(other.reparsePoint),
      compressed(other.compressed), encrypted(other.encrypted),
      offline(other.offline), notContentIndexed(other.notContentIndexed),
      virtualFile(other.virtualFile) {
}

FileAttributes& FileAttributes::operator=(FileAttributes&& other) noexcept {
    if (this != &other) {
        readOnly = other.readOnly;
        hidden = other.hidden;
        system = other.system;
        directory = other.directory;
        archive = other.archive;
        temporary = other.temporary;
        sparse = other.sparse;
        reparsePoint = other.reparsePoint;
        compressed = other.compressed;
        encrypted = other.encrypted;
        offline = other.offline;
        notContentIndexed = other.notContentIndexed;
        virtualFile = other.virtualFile;
    }
    return *this;
}

FileAttributes::~FileAttributes() {
}

// FileTimestamps implementation
FileTimestamps::FileTimestamps() 
    : creationTime(0), lastWriteTime(0), lastAccessTime(0), changeTime(0) {
}

FileTimestamps::FileTimestamps(const FileTimestamps& other)
    : creationTime(other.creationTime), lastWriteTime(other.lastWriteTime),
      lastAccessTime(other.lastAccessTime), changeTime(other.changeTime) {
}

FileTimestamps& FileTimestamps::operator=(const FileTimestamps& other) {
    if (this != &other) {
        creationTime = other.creationTime;
        lastWriteTime = other.lastWriteTime;
        lastAccessTime = other.lastAccessTime;
        changeTime = other.changeTime;
    }
    return *this;
}

FileTimestamps::FileTimestamps(FileTimestamps&& other) noexcept
    : creationTime(other.creationTime), lastWriteTime(other.lastWriteTime),
      lastAccessTime(other.lastAccessTime), changeTime(other.changeTime) {
}

FileTimestamps& FileTimestamps::operator=(FileTimestamps&& other) noexcept {
    if (this != &other) {
        creationTime = other.creationTime;
        lastWriteTime = other.lastWriteTime;
        lastAccessTime = other.lastAccessTime;
        changeTime = other.changeTime;
    }
    return *this;
}

FileTimestamps::~FileTimestamps() {
}

// FileChunk implementation
FileChunk::FileChunk(uint64_t off, uint64_t len) 
    : offset(off), length(len) {
}

FileChunk::FileChunk(const FileChunk& other)
    : offset(other.offset), length(other.length), hash(other.hash) {
}

FileChunk& FileChunk::operator=(const FileChunk& other) {
    if (this != &other) {
        offset = other.offset;
        length = other.length;
        hash = other.hash;
    }
    return *this;
}

FileChunk::FileChunk(FileChunk&& other) noexcept
    : offset(other.offset), length(other.length), hash(std::move(other.hash)) {
    other.offset = 0;
    other.length = 0;
}

FileChunk& FileChunk::operator=(FileChunk&& other) noexcept {
    if (this != &other) {
        offset = other.offset;
        length = other.length;
        hash = std::move(other.hash);
        
        other.offset = 0;
        other.length = 0;
    }
    return *this;
}

FileChunk::~FileChunk() {
}