#include "dedupe.h"
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include "libs/chash/blake3.h"
#include "libs/utils/utils.h"
#include "core/safety/safety.h"
#include "platform/util/trash.h"

Deduplicator::Deduplicator(LSMIndex& index) : m_index(index) {
}

Deduplicator::~Deduplicator() {
}

std::vector<DuplicateGroup> Deduplicator::findDuplicates(const DedupeOptions& options) {
    std::vector<DuplicateGroup> groups;
    
    // Reset statistics
    m_stats = DedupeStats();
    
    // Group files by size (first filter)
    auto sizeGroups = groupBySize();
    
    // Process each size group
    for (const auto& [size, files] : sizeGroups) {
        if (size < options.minFileSize) {
            continue;
        }
        
        // Skip groups with only one file
        if (files.size() < 2) {
            continue;
        }
        
        m_stats.totalFiles += files.size();
        
        // Filter by head/tail signatures
        auto filteredFiles = filterByHeadTail(files);
        
        // Skip groups with only one file after filtering
        if (filteredFiles.size() < 2) {
            continue;
        }
        
        // Compute full hashes if requested or if group is still large
        std::vector<FileEntry> hashVerifiedFiles;
        if (options.computeFullHash || filteredFiles.size() > 10) {
            hashVerifiedFiles = computeFullHashes(filteredFiles);
        } else {
            hashVerifiedFiles = filteredFiles;
        }
        
        // Group by hash
        auto hashGroups = groupByHash(hashVerifiedFiles);
        
        // Create duplicate groups
        for (const auto& [hash, groupFiles] : hashGroups) {
            if (groupFiles.size() < 2) {
                continue;
            }
            
            DuplicateGroup group;
            group.files = groupFiles;
            group.potentialSavings = (groupFiles.size() - 1) * size;
            
            groups.push_back(group);
            m_stats.duplicateGroups++;
            m_stats.duplicateFiles += groupFiles.size();
            m_stats.potentialSavings += group.potentialSavings;
        }
    }
    
    return groups;
}

DedupeStats Deduplicator::deduplicate(const std::vector<DuplicateGroup>& groups, const DedupeOptions& options) {
    // Reset actual savings counter
    m_stats.actualSavings = 0;
    m_stats.hardlinksCreated = 0;
    
    // Process each group
    for (const auto& group : groups) {
        if (group.files.size() < 2) {
            continue;
        }
        
        // Enforce Safety Mode: if deletions are not allowed, only simulate or use non-destructive ops
        const bool safety_blocks_delete = !safety::deletion_allowed();
        if (options.simulateOnly || safety_blocks_delete) {
            // Just update stats
            m_stats.actualSavings += group.potentialSavings;
            continue;
        }
        
        // Determine action based on options
        if (options.useHardlinks && areOnSameVolume(group.files)) {
            // Create hardlinks
            if (createHardlinks(group.files)) {
                m_stats.actualSavings += group.potentialSavings;
                m_stats.hardlinksCreated += group.files.size() - 1;
            }
        } else if (options.moveToRecycleBin) {
            // Move duplicates to recycle bin
            if (moveToRecycleBin({group.files.begin() + 1, group.files.end()})) {
                m_stats.actualSavings += group.potentialSavings;
            }
        } else {
            // Delete duplicates (only if allowed by Safety Mode)
            if (!safety_blocks_delete) {
                if (deleteFiles({group.files.begin() + 1, group.files.end()})) {
                    m_stats.actualSavings += group.potentialSavings;
                }
            }
        }
    }
    
    return m_stats;
}

std::map<uint64_t, std::vector<FileEntry>> Deduplicator::groupBySize() const {
    std::map<uint64_t, std::vector<FileEntry>> sizeGroups;
    
    // In a real implementation, we would iterate through the index
    // For now, we'll return an empty map as a placeholder
    return sizeGroups;
}

std::vector<FileEntry> Deduplicator::filterByHeadTail(const std::vector<FileEntry>& candidates) const {
    std::unordered_map<std::string, std::vector<FileEntry>> signatureGroups;
    
    // Group by head/tail signature
    for (const auto& file : candidates) {
        if (file.headTail16.has_value()) {
            std::string signature(file.headTail16->begin(), file.headTail16->end());
            signatureGroups[signature].push_back(file);
        }
    }
    
    // Collect files that have duplicates
    std::vector<FileEntry> result;
    for (const auto& [signature, files] : signatureGroups) {
        if (files.size() > 1) {
            result.insert(result.end(), files.begin(), files.end());
        }
    }
    
    return result;
}

std::vector<FileEntry> Deduplicator::computeFullHashes(const std::vector<FileEntry>& candidates) const {
    // Streaming BLAKE3 hashing per file using FileUtils, with small read buffer to cap RAM and I/O backpressure
    constexpr size_t kBufSize = 256 * 1024; // 256 KiB
    std::vector<uint8_t> buffer(kBufSize);

    std::vector<FileEntry> withHashes; withHashes.reserve(candidates.size());

    for (auto fe : candidates) { // copy; we will set sha256 on the copy
        if (fe.sha256.has_value() && !fe.sha256->empty()) {
            withHashes.push_back(std::move(fe));
            continue;
        }

        // Open file read-only
        file_handle_t fh = FileUtils::open_file(fe.fullPath, /*read_only*/ true);
        if (!FileUtils::is_valid_handle(fh)) {
            // Skip unreadable files
            continue;
        }

        BLAKE3_HASH_STATE st; blake3_hash_init(&st);
        uint64_t offset = 0;
        bool ok = true;
        while (offset < fe.sizeLogical) {
            size_t toRead = static_cast<size_t>(std::min<uint64_t>(kBufSize, fe.sizeLogical - offset));
            if (!FileUtils::read_file_data(fh, buffer.data(), toRead, offset)) {
                ok = false; break;
            }
            blake3_hash_update(&st, buffer.data(), toRead);
            offset += toRead;
        }
        FileUtils::close_file(fh);
        if (!ok) {
            // Skip if read failed mid-way
            continue;
        }
        std::vector<uint8_t> digest(BLAKE3_OUT_LEN);
        blake3_hash_finalize(&st, digest.data(), BLAKE3_OUT_LEN);
        fe.sha256 = std::move(digest); // reuse sha256 field to store full-file BLAKE3 digest
        withHashes.push_back(std::move(fe));
    }

    // Devolver solo aquellos con hash calculado
    return withHashes;
}

std::map<std::string, std::vector<FileEntry>> Deduplicator::groupByHash(const std::vector<FileEntry>& files) const {
    std::map<std::string, std::vector<FileEntry>> hashGroups;
    
    // Group by full hash
    for (const auto& file : files) {
        if (file.sha256.has_value()) {
            std::string hash(file.sha256->begin(), file.sha256->end());
            hashGroups[hash].push_back(file);
        } else if (file.headTail16.has_value()) {
            // Fallback to head/tail if full hash is not available
            std::string hash(file.headTail16->begin(), file.headTail16->end());
            hashGroups[hash].push_back(file);
        }
    }
    
    return hashGroups;
}

bool Deduplicator::createHardlinks(const std::vector<FileEntry>& group) {
    if (group.size() < 2) {
        return false;
    }
    
    // Use the first file as the source
    const FileEntry& source = group[0];
    
    // Create hardlinks for the rest
    bool success = true;
    for (size_t i = 1; i < group.size(); i++) {
        const FileEntry& target = group[i];
        
        // In a real implementation, we would create the actual hardlink
        // For now, we'll just simulate the operation
#ifdef _WIN32
        // Windows implementation would use CreateHardLinkW
        // BOOL result = CreateHardLinkW(targetPath.c_str(), sourcePath.c_str(), nullptr);
        // success &= (result != FALSE);
#else
        // Linux implementation would use link()
        // int result = link(sourcePath.c_str(), targetPath.c_str());
        // success &= (result == 0);
#endif
    }
    
    return success;
}

bool Deduplicator::moveToRecycleBin(const std::vector<FileEntry>& files) {
    bool allOk = true;
    for (const auto& fe : files) {
        std::string trashed;
        if (!platform::move_to_trash(fe.fullPath, &trashed)) {
            allOk = false;
        }
    }
    return allOk;
}

bool Deduplicator::deleteFiles(const std::vector<FileEntry>& files) {
    // In a real implementation, we would delete the files
    // For now, we'll just simulate the operation
    return true;
}

bool Deduplicator::areOnSameVolume(const std::vector<FileEntry>& files) const {
    if (files.empty()) {
        return false;
    }
    
    VolumeId firstVolume = files[0].volumeId;
    for (const auto& file : files) {
        if (file.volumeId != firstVolume) {
            return false;
        }
    }
    
    return true;
}

void Deduplicator::generateUndoInfo(const std::vector<FileEntry>& originalFiles, 
                                   const std::vector<FileEntry>& newFiles) {
    // In a real implementation, we would generate undo information
    // to allow reverting the deduplication operation
}
