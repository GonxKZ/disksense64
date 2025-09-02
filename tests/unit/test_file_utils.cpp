#include "file_utils.h"
#include <cassert>
#include <iostream>
#include <fstream>

int test_file_utils() {
    std::cout << "Testing cross-platform file utilities..." << std::endl;
    
    // Test path separator
    char separator = FileUtils::get_path_separator();
#ifdef _WIN32
    assert(separator == '\\');
#else
    assert(separator == '/');
#endif
    std::cout << "Path separator test passed: " << separator << std::endl;
    
    // Test path joining
    std::string path1 = "home";
    std::string path2 = "user";
    std::string joined = FileUtils::join_paths(path1, path2);
    
#ifdef _WIN32
    assert(joined == "home\\user");
#else
    assert(joined == "home/user");
#endif
    std::cout << "Path joining test passed: " << joined << std::endl;
    
    // Test file extension
    std::string filename = "test.txt";
    std::string ext = FileUtils::get_file_extension(filename);
    assert(ext == ".txt");
    std::cout << "File extension test passed: " << ext << std::endl;
    
    // Test platform path conversion
    std::string unix_path = "/home/user/documents";
    std::string platform_path = FileUtils::to_platform_path(unix_path);
    
#ifdef _WIN32
    assert(platform_path == "\\home\\user\\documents");
#else
    assert(platform_path == "/home/user/documents");
#endif
    std::cout << "Platform path conversion test passed: " << platform_path << std::endl;
    
    // Test directory operations
    std::string temp_dir = FileUtils::get_temp_directory();
    assert(!temp_dir.empty());
    std::cout << "Temp directory test passed: " << temp_dir << std::endl;
    
    std::string home_dir = FileUtils::get_home_directory();
    assert(!home_dir.empty());
    std::cout << "Home directory test passed: " << home_dir << std::endl;
    
    // Test file operations
    std::string test_file = FileUtils::join_paths(temp_dir, "test_file.txt");
    
    // Create test file
    std::ofstream ofs(test_file);
    ofs << "This is a test file for cross-platform file utilities.";
    ofs.close();
    
    // Test file existence
    assert(FileUtils::path_exists(test_file));
    std::cout << "File existence test passed" << std::endl;
    
    // Test file info
    file_info_t info;
    assert(FileUtils::get_file_info(test_file, info));
    assert(info.size > 0);
    assert(!info.is_directory);
    std::cout << "File info test passed: " << info.size << " bytes" << std::endl;
    
    // Test file deletion
    assert(FileUtils::delete_file(test_file));
    assert(!FileUtils::path_exists(test_file));
    std::cout << "File deletion test passed" << std::endl;
    
    // Test directory listing
    std::vector<file_info_t> files = FileUtils::list_directory(temp_dir);
    std::cout << "Directory listing test passed: " << files.size() << " items found" << std::endl;
    
    std::cout << "All file utility tests passed!" << std::endl;
    return 0;
}

int main() {
    return test_file_utils();
}