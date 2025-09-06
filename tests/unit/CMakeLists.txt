option(MINIMAL_UNIT_TESTS "Build only a minimal, stable subset of unit tests" OFF)

if(MINIMAL_UNIT_TESTS)
    # Minimal set: hashing, utils and safety ops
    add_executable(test_sha256 test_sha256.cpp)
    target_link_libraries(test_sha256 PRIVATE lib_chash)
    target_include_directories(test_sha256 PRIVATE ../../libs/chash)
    add_test(NAME test_sha256 COMMAND test_sha256)

    add_executable(test_utils test_utils.cpp)
    target_link_libraries(test_utils PRIVATE lib_utils)
    add_test(NAME test_utils COMMAND test_utils)

    add_executable(test_safety_ops ops/test_safety.cpp)
    target_link_libraries(test_safety_ops PRIVATE core_ops lib_utils)
    target_include_directories(test_safety_ops PRIVATE 
        ../../core/ops
        ../../core/safety
        ../../libs/utils
    )
    add_test(NAME test_safety_ops COMMAND test_safety_ops)

    add_executable(test_dedupe_safety ops/test_dedupe_safety.cpp)
    target_link_libraries(test_dedupe_safety PRIVATE core_ops lib_utils core_index)
    target_include_directories(test_dedupe_safety PRIVATE 
        ../../core/ops
        ../../core/index
        ../../core/model
        ../../core/safety
        ../../libs/utils
    )
    add_test(NAME test_dedupe_safety COMMAND test_dedupe_safety)
    add_executable(test_dedupe_hash ops/test_dedupe_hash.cpp)
    target_link_libraries(test_dedupe_hash PRIVATE core_ops core_index lib_utils lib_chash)
    target_include_directories(test_dedupe_hash PRIVATE 
        ../../core/ops
        ../../core/index
        ../../core/model
        ../../libs/utils
        ../../libs/chash)
    add_test(NAME test_dedupe_hash COMMAND test_dedupe_hash)

    # Trash move/list/restore (cross-platform)
    add_executable(test_trash platform/test_trash.cpp)
    target_link_libraries(test_trash PRIVATE platform_util)
    add_test(NAME test_trash COMMAND test_trash)

    # Windows MFT enumerator
    if(WIN32)
        add_executable(test_mft scan/test_mft.cpp)
        target_link_libraries(test_mft PRIVATE core_scan)
        add_test(NAME test_mft COMMAND test_mft)
    endif()

else()

    # Full suite (original)
    # SHA256 tests
    add_executable(test_sha256 test_sha256.cpp)
    target_link_libraries(test_sha256 PRIVATE lib_chash)
    target_include_directories(test_sha256 PRIVATE ../../libs/chash)
    add_test(NAME test_sha256 COMMAND test_sha256)

# Cross-platform utilities tests
add_executable(test_utils test_utils.cpp)
target_link_libraries(test_utils PRIVATE lib_utils)
add_test(NAME test_utils COMMAND test_utils)

# Fuzzy hashing tests
add_executable(test_fuzzy_hash fuzzyhash/test_fuzzy_hash.cpp)
target_link_libraries(test_fuzzy_hash PRIVATE lib_fuzzyhash lib_utils)
target_include_directories(test_fuzzy_hash PRIVATE 
    ../../libs/fuzzyhash
    ../../libs/utils
)
add_test(NAME test_fuzzy_hash COMMAND test_fuzzy_hash)

# Metadata analysis tests
if(NOT MINIMAL_UNIT_TESTS)
add_executable(test_metadata metadata/test_metadata.cpp)
target_link_libraries(test_metadata PRIVATE lib_metadata lib_utils)
target_include_directories(test_metadata PRIVATE 
    ../../libs/metadata
    ../../libs/utils
)
add_test(NAME test_metadata COMMAND test_metadata)
endif()

# File carving tests
if(NOT MINIMAL_UNIT_TESTS)
add_executable(test_carving carving/test_carving.cpp)
target_link_libraries(test_carving PRIVATE lib_carving lib_utils)
target_include_directories(test_carving PRIVATE 
    ../../libs/carving
    ../../libs/utils
)
add_test(NAME test_carving COMMAND test_carving)
endif()

# Hidden files and rootkits detection tests
if(NOT MINIMAL_UNIT_TESTS)
add_executable(test_hidden hidden/test_hidden.cpp)
target_link_libraries(test_hidden PRIVATE lib_hidden lib_utils lib_metadata)
target_include_directories(test_hidden PRIVATE 
    ../../libs/hidden
    ../../libs/utils
    ../../libs/metadata
)
add_test(NAME test_hidden COMMAND test_hidden)
endif()

# Slack space and unallocated space analysis tests
if(NOT MINIMAL_UNIT_TESTS)
add_executable(test_slack slack/test_slack.cpp)
target_link_libraries(test_slack PRIVATE lib_slack lib_utils)
target_include_directories(test_slack PRIVATE 
    ../../libs/slack
    ../../libs/utils
)
add_test(NAME test_slack COMMAND test_slack)
endif()

# Forensic image mounting tests
if(NOT MINIMAL_UNIT_TESTS)
add_executable(test_mount mount/test_mount.cpp)
target_link_libraries(test_mount PRIVATE lib_mount lib_utils)
target_include_directories(test_mount PRIVATE 
    ../../libs/mount
    ../../libs/utils
)
add_test(NAME test_mount COMMAND test_mount)
endif()

# System logs analysis tests
if(NOT MINIMAL_UNIT_TESTS)
add_executable(test_logs logs/test_logs.cpp)
target_link_libraries(test_logs PRIVATE lib_logs lib_utils)
target_include_directories(test_logs PRIVATE 
    ../../libs/logs
    ../../libs/utils
)
add_test(NAME test_logs COMMAND test_logs)
endif()

# YARA malware detection tests
if(NOT MINIMAL_UNIT_TESTS)
add_executable(test_yara yara/test_yara.cpp)
target_link_libraries(test_yara PRIVATE lib_yara lib_utils)
target_include_directories(test_yara PRIVATE 
    ../../libs/yara
    ../../libs/utils
)
add_test(NAME test_yara COMMAND test_yara)
endif()

# File clustering tests
if(NOT MINIMAL_UNIT_TESTS)
add_executable(test_clustering clustering/test_clustering.cpp)
target_link_libraries(test_clustering PRIVATE lib_clustering lib_utils lib_fuzzyhash)
target_include_directories(test_clustering PRIVATE 
    ../../libs/clustering
    ../../libs/utils
    ../../libs/fuzzyhash
)
add_test(NAME test_clustering COMMAND test_clustering)
endif()

# Compression analysis tests
add_executable(test_compression compression/test_compression.cpp)
target_link_libraries(test_compression PRIVATE lib_compression lib_utils)
target_include_directories(test_compression PRIVATE 
    ../../libs/compression
    ../../libs/utils
)
add_test(NAME test_compression COMMAND test_compression)

# Encryption analysis tests
if(NOT MINIMAL_UNIT_TESTS)
add_executable(test_encryption encryption/test_encryption.cpp)
target_link_libraries(test_encryption PRIVATE lib_encryption lib_utils)
target_include_directories(test_encryption PRIVATE 
    ../../libs/encryption
    ../../libs/utils
)
add_test(NAME test_encryption COMMAND test_encryption)
endif()

# Network traffic analysis tests
if(NOT MINIMAL_UNIT_TESTS)
add_executable(test_network network/test_network.cpp)
target_link_libraries(test_network PRIVATE lib_network lib_utils)
target_include_directories(test_network PRIVATE 
    ../../libs/network
    ../../libs/utils
)
add_test(NAME test_network COMMAND test_network)
endif()

# Machine learning tests
if(NOT MINIMAL_UNIT_TESTS)
add_executable(test_ml ml/test_ml.cpp)
target_link_libraries(test_ml PRIVATE lib_ml lib_utils lib_fuzzyhash lib_metadata)
target_include_directories(test_ml PRIVATE 
    ../../libs/ml
    ../../libs/utils
    ../../libs/fuzzyhash
    ../../libs/metadata
)
add_test(NAME test_ml COMMAND test_ml)
endif()

# Memory analysis tests
if(NOT MINIMAL_UNIT_TESTS)
add_executable(test_memory memory/test_memory.cpp)
target_link_libraries(test_memory PRIVATE lib_memory lib_utils)
target_include_directories(test_memory PRIVATE 
    ../../libs/memory
    ../../libs/utils
)
add_test(NAME test_memory COMMAND test_memory)
endif()

# Security audit tests
    add_executable(test_security security/test_security.cpp)
    target_link_libraries(test_security PRIVATE lib_security lib_utils)
    target_include_directories(test_security PRIVATE 
        ../../libs/security
        ../../libs/utils
    )
    add_test(NAME test_security COMMAND test_security)

    # Add more tests here as modules are implemented

    # Safety and ops tests
    add_executable(test_safety_ops ops/test_safety.cpp)
    target_link_libraries(test_safety_ops PRIVATE core_ops lib_utils)
    target_include_directories(test_safety_ops PRIVATE 
        ../../core/ops
        ../../core/safety
        ../../libs/utils
    )
    add_test(NAME test_safety_ops COMMAND test_safety_ops)

    add_executable(test_dedupe_safety ops/test_dedupe_safety.cpp)
    target_link_libraries(test_dedupe_safety PRIVATE core_ops lib_utils core_index)
    target_include_directories(test_dedupe_safety PRIVATE 
        ../../core/ops
        ../../core/index
        ../../core/model
        ../../core/safety
        ../../libs/utils
    )
    add_test(NAME test_dedupe_safety COMMAND test_dedupe_safety)

endif()
