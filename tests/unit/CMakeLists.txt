# SHA256 tests
add_executable(test_sha256 test_sha256.cpp)
target_link_libraries(test_sha256 PRIVATE lib_chash)
target_include_directories(test_sha256 PRIVATE ../../libs/chash)
add_test(NAME test_sha256 COMMAND test_sha256)

# Cross-platform utilities tests
add_executable(test_utils test_utils.cpp)
target_link_libraries(test_utils PRIVATE lib_utils)
add_test(NAME test_utils COMMAND test_utils)

# Add more tests here as modules are implemented
