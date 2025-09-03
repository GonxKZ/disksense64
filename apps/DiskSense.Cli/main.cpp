#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "core/scan/scanner.h"
#include "core/index/lsm_index.h"
#include "core/ops/dedupe.h"
#include "core/model/model.h"
#include "libs/utils/utils.h"

void printUsage(const char* programName) {
    std::cout << "DiskSense64 - Cross-Platform Disk Analysis Suite" << std::endl;
    std::cout << "Version: 1.0.0" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: " << programName << " <command> [options] <directory>" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  scan     - Scan directory and build index" << std::endl;
    std::cout << "  dedupe   - Find and remove duplicates" << std::endl;
    std::cout << "  similar  - Find similar files (images/audio)" << std::endl;
    std::cout << "  cleanup  - Clean residue files" << std::endl;
    std::cout << "  treemap  - Generate treemap visualization (GUI only)" << std::endl;
    std::cout << std::endl;
    std::cout << "Options for dedupe:" << std::endl;
    std::cout << "  --action=<simulate|hardlink|move|delete>  Action to perform (default: simulate)" << std::endl;
    std::cout << "  --min-size=<bytes>                        Minimum file size to consider (default: 1024)" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " scan /home/user/Documents" << std::endl;
    std::cout << "  " << programName << " dedupe --action=hardlink /home/user/Downloads" << std::endl;
    std::cout << "  " << programName << " similar /home/user/Pictures" << std::endl;
}

std::string getIndexPath(const std::string& directory) {
    // Create .disksense64 directory in user's home or in the directory
    std::string homeDir;
    
#ifdef _WIN32
    char* homedrive = getenv("HOMEDRIVE");
    char* homepath = getenv("HOMEPATH");
    if (homedrive && homepath) {
        homeDir = std::string(homedrive) + std::string(homepath);
    } else {
        homeDir = directory;
    }
#else
    char* home = getenv("HOME");
    if (home) {
        homeDir = std::string(home);
    } else {
        homeDir = directory;
    }
#endif
    
    return FileUtils::join_paths(homeDir, ".disksense64");
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    std::string command(argv[1]);
    std::string path_str(argv[2]);
    
    // Convert to platform-specific path
    std::string platform_path = FileUtils::to_platform_path(path_str);
    
    // Get index path
    std::string index_path = getIndexPath(platform_path);

    std::cout << "DiskSense64 - Starting analysis of: " << platform_path << std::endl;
    std::cout << "Index path: " << index_path << std::endl;
    std::cout << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    if (command == "scan") {
        // Create index
        LSMIndex index(index_path);
        
        // Scan directory
        Scanner scanner;
        ScanOptions options;
        options.computeHeadTail = true;
        options.computeFullHash = false;
        
        uint64_t file_count = 0;
        scanner.scanVolume(platform_path, options, 
                          [&index, &file_count](const ScanEvent& event) {
            if (event.type == ScanEventType::FileAdded) {
                index.put(event.fileEntry);
                file_count++;
                
                if (file_count % 1000 == 0) {
                    std::cout << "Processed " << file_count << " files...\r" << std::flush;
                }
            }
        });
        
        // Flush index to disk
        index.flush();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "Scan completed! Processed " << file_count << " files in " 
                  << duration.count() << " ms." << std::endl;
        std::cout << "Index saved to: " << index_path << std::endl;
    } 
    else if (command == "dedupe") {
        // Load index
        LSMIndex index(index_path);
        
        // Parse options
        DedupeOptions options;
        options.simulateOnly = true; // For safety, only simulate by default
        options.useHardlinks = true;
        options.minFileSize = 1024; // 1KB minimum
        
        // Parse command line arguments for options
        for (int i = 3; i < argc; i++) {
            std::string arg(argv[i]);
            if (arg == "--action=simulate") {
                options.simulateOnly = true;
            } else if (arg == "--action=hardlink") {
                options.simulateOnly = false;
                options.useHardlinks = true;
            } else if (arg == "--action=move") {
                options.simulateOnly = false;
                options.moveToRecycleBin = true;
            } else if (arg == "--action=delete") {
                options.simulateOnly = false;
                options.moveToRecycleBin = false;
            } else if (arg.rfind("--min-size=", 0) == 0) {
                try {
                    options.minFileSize = std::stoull(arg.substr(11));
                } catch (...) {
                    std::cerr << "Invalid min-size value: " << arg << std::endl;
                    return 1;
                }
            }
        }
        
        std::cout << "Finding duplicates with minimum size: " << options.minFileSize << " bytes" << std::endl;
        if (options.simulateOnly) {
            std::cout << "Running in simulation mode (no changes will be made)" << std::endl;
        } else if (options.useHardlinks) {
            std::cout << "Will create hardlinks for duplicates" << std::endl;
        } else if (options.moveToRecycleBin) {
            std::cout << "Will move duplicates to recycle bin" << std::endl;
        } else {
            std::cout << "Will delete duplicates" << std::endl;
        }
        std::cout << std::endl;
        
        // Find duplicates
        Deduplicator deduper(index);
        auto groups = deduper.findDuplicates(options);
        
        std::cout << "Found " << groups.size() << " duplicate groups." << std::endl;
        
        // Print statistics
        const auto& stats = deduper.getStats();
        std::cout << "Total files analyzed: " << stats.totalFiles << std::endl;
        std::cout << "Duplicate files found: " << stats.duplicateFiles << std::endl;
        std::cout << "Potential space savings: " << stats.potentialSavings << " bytes (" 
                  << (stats.potentialSavings / (1024.0 * 1024.0)) << " MB)" << std::endl;
        
        if (!options.simulateOnly && !groups.empty()) {
            std::cout << std::endl;
            std::cout << "Performing deduplication..." << std::endl;
            
            // Perform actual deduplication
            auto finalStats = deduper.deduplicate(groups, options);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            std::cout << "Deduplication completed in " << duration.count() << " ms." << std::endl;
            std::cout << "Actual space savings: " << finalStats.actualSavings << " bytes (" 
                      << (finalStats.actualSavings / (1024.0 * 1024.0)) << " MB)" << std::endl;
            if (finalStats.hardlinksCreated > 0) {
                std::cout << "Hardlinks created: " << finalStats.hardlinksCreated << std::endl;
            }
        } else if (groups.empty()) {
            std::cout << "No duplicates found in the specified directory." << std::endl;
        }
    }
    else if (command == "similar") {
        std::cout << "Similarity detection feature not yet implemented in this version." << std::endl;
        std::cout << "This feature will be available in a future release." << std::endl;
    }
    else if (command == "cleanup") {
        std::cout << "Residue cleanup feature not yet implemented in this version." << std::endl;
        std::cout << "This feature will be available in a future release." << std::endl;
    }
    else if (command == "treemap") {
        std::cout << "Treemap visualization not available in CLI." << std::endl;
        std::cout << "Use the GUI application for visualization." << std::endl;
    }
    else {
        std::cerr << "Unknown command: " << command << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
