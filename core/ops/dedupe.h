#ifndef CORE_OPS_DEDUPE_H
#define CORE_OPS_DEDUPE_H

#include <vector>
#include <map>
#include <set>
#include <string>
#include <memory>
#include "core/model/model.h"
#include "core/index/lsm_index.h"

// Duplicate detection result
struct DuplicateGroup {
    std::vector<FileEntry> files;
    uint64_t potentialSavings; // Bytes that could be saved by deduplication
    
    DuplicateGroup() : potentialSavings(0) {}
};

// Deduplication statistics
struct DedupeStats {
    uint64_t totalFiles;
    uint64_t duplicateGroups;
    uint64_t duplicateFiles;
    uint64_t potentialSavings; // Bytes
    uint64_t actualSavings;    // Bytes actually saved
    uint64_t hardlinksCreated;
    
    DedupeStats() : totalFiles(0), duplicateGroups(0), duplicateFiles(0),
                    potentialSavings(0), actualSavings(0), hardlinksCreated(0) {}
};

// Deduplication options
struct DedupeOptions {
    bool simulateOnly = true;          // Only simulate, don't actually deduplicate
    bool useHardlinks = false;         // Create hardlinks for duplicates on same volume
    bool moveToRecycleBin = false;     // Move duplicates to recycle bin instead of deleting
    bool computeFullHash = false;      // Compute full hash for all candidates (slower but more accurate)
    uint64_t minFileSize = 1024;       // Minimum file size to consider for deduplication
    std::vector<std::wstring> excludePaths; // Paths to exclude from deduplication
};

// Deduplication module
class Deduplicator {
public:
    Deduplicator(LSMIndex& index);
    ~Deduplicator();
    
    // Find duplicate files
    std::vector<DuplicateGroup> findDuplicates(const DedupeOptions& options);
    
    // Deduplicate files (perform actual deduplication)
    DedupeStats deduplicate(const std::vector<DuplicateGroup>& groups, const DedupeOptions& options);
    
    // Get current statistics
    const DedupeStats& getStats() const { return m_stats; }
    
private:
    LSMIndex& m_index;
    DedupeStats m_stats;
    
    // Group files by size
    std::map<uint64_t, std::vector<FileEntry>> groupBySize() const;
    
    // Filter candidates using head/tail signatures
    std::vector<FileEntry> filterByHeadTail(const std::vector<FileEntry>& candidates) const;
    
    // Compute full hashes for final verification
    std::vector<FileEntry> computeFullHashes(const std::vector<FileEntry>& candidates) const;
    
    // Group files by hash
    std::map<std::string, std::vector<FileEntry>> groupByHash(const std::vector<FileEntry>& files) const;
    
    // Create hardlinks for duplicates
    bool createHardlinks(const std::vector<FileEntry>& group);
    
    // Move files to recycle bin
    bool moveToRecycleBin(const std::vector<FileEntry>& files);
    
    // Delete files
    bool deleteFiles(const std::vector<FileEntry>& files);
    
    // Check if files are on the same volume
    bool areOnSameVolume(const std::vector<FileEntry>& files) const;
    
    // Generate undo information
    void generateUndoInfo(const std::vector<FileEntry>& originalFiles, 
                         const std::vector<FileEntry>& newFiles);
};

#endif // CORE_OPS_DEDUPE_H