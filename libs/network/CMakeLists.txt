# Network traffic analysis library
add_library(lib_network
    network.c
    network.h
    capture.c
    capture.h
    protocols.c
    protocols.h
    analysis.c
    analysis.h
)

target_include_directories(lib_network PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Find pcap library for packet capture (prefer pkg-config)
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PCAP QUIET libpcap)
endif()

if(PCAP_FOUND)
    message(STATUS "PCAP_INCLUDE_DIRS: ${PCAP_INCLUDE_DIRS}")
    target_include_directories(lib_network PUBLIC ${PCAP_INCLUDE_DIRS})
    target_link_libraries(lib_network PRIVATE ${PCAP_LIBRARIES})
else()
    # Fallback: try plain -lpcap
    find_path(PCAP_INCLUDE_DIR pcap/pcap.h)
    find_library(PCAP_LIBRARY pcap)
    if(PCAP_INCLUDE_DIR)
        target_include_directories(lib_network PUBLIC ${PCAP_INCLUDE_DIR})
    endif()
    if(PCAP_LIBRARY)
        target_link_libraries(lib_network PRIVATE ${PCAP_LIBRARY})
        set(PCAP_FOUND TRUE)
    else()
        message(WARNING "libpcap not found; network capture features will be limited")
    endif()
endif()

target_link_libraries(lib_network 
    PRIVATE lib_utils
)

# Define library alias
add_library(network::network ALIAS lib_network)
