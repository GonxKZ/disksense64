#include "libs/utils/utils.h"
#include <cassert>
#include <iostream>

int test_file_utils() {
    // Test path separator
    char separator = FileUtils::get_path_separator();
    
#ifdef _WIN32
    assert(separator == '\\');
#else
    assert(separator == '/');
#endif
    
    // Test path joining
    std::string path1 = "home";
    std::string path2 = "user";
    std::string joined = FileUtils::join_paths(path1, path2);
    
#ifdef _WIN32
    assert(joined == "home\\user");
#else
    assert(joined == "home/user");
#endif
    
    // Test file extension
    std::string filename = "test.txt";
    std::string ext = FileUtils::get_file_extension(filename);
    assert(ext == ".txt");
    
    std::cout << "FileUtils tests passed!" << std::endl;
    return 0;
}

int test_string_utils() {
    // Test case insensitive comparison
    std::string str1 = "Hello";
    std::string str2 = "HELLO";
    assert(StringUtils::iequals(str1, str2));
    
    // Test to lower
    std::string upper = "HELLO WORLD";
    std::string lower = StringUtils::to_lower(upper);
    assert(lower == "hello world");
    
    // Test splitting
    std::string csv = "a,b,c";
    auto parts = StringUtils::split(csv, ',');
    assert(parts.size() == 3);
    assert(parts[0] == "a");
    assert(parts[1] == "b");
    assert(parts[2] == "c");
    
    std::cout << "StringUtils tests passed!" << std::endl;
    return 0;
}

int test_time_utils() {
    // Test current time
    uint64_t time1 = TimeUtils::get_current_time_ms();
    uint64_t time2 = TimeUtils::get_current_time_ms();
    
    // Should be within a reasonable range
    assert(time2 >= time1);
    assert(time2 - time1 < 1000); // Less than 1 second difference
    
    std::cout << "TimeUtils tests passed!" << std::endl;
    return 0;
}

int test_system_utils() {
    // Test CPU cores
    unsigned int cores = SystemUtils::get_cpu_cores();
    assert(cores > 0);
    
    // Test memory
    uint64_t total_memory = SystemUtils::get_total_memory();
    uint64_t available_memory = SystemUtils::get_available_memory();
    assert(total_memory > 0);
    assert(available_memory > 0);
    assert(available_memory <= total_memory);
    
    // Test page size
    size_t page_size = SystemUtils::get_page_size();
    assert(page_size > 0);
    
    std::cout << "SystemUtils tests passed!" << std::endl;
    return 0;
}

int main() {
    std::cout << "Running cross-platform utility tests..." << std::endl;
    
    int result = 0;
    result |= test_file_utils();
    result |= test_string_utils();
    result |= test_time_utils();
    result |= test_system_utils();
    
    if (result == 0) {
        std::cout << "All tests passed!" << std::endl;
    } else {
        std::cout << "Some tests failed!" << std::endl;
    }
    
    return result;
}
