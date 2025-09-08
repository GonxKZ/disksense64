# Platform module for DiskSense64

# Create platform library
add_library(platform
    platformmanager.h
    platformmanager.cpp
    i18n.h
    i18n.cpp
)

target_include_directories(platform PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(platform PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
)

# Platform-specific definitions
if(WIN32)
    target_compile_definitions(platform PRIVATE PLATFORM_WINDOWS)
elseif(APPLE)
    target_compile_definitions(platform PRIVATE PLATFORM_MACOS)
else()
    target_compile_definitions(platform PRIVATE PLATFORM_LINUX)
endif()

# Add translation files to resources
set(TRANSLATION_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/disksense_en.qm
    ${CMAKE_CURRENT_SOURCE_DIR}/disksense_es.qm
)

# Create resource file for translations
configure_file(${CMAKE_SOURCE_DIR}/platform/translations.qrc.in 
               ${CMAKE_BINARY_DIR}/platform/translations.qrc)

qt6_add_resources(TRANSLATION_RESOURCES ${CMAKE_BINARY_DIR}/platform/translations.qrc)

# Add translation resources to platform library
target_sources(platform PRIVATE ${TRANSLATION_RESOURCES})

# Install translation files
install(FILES ${TRANSLATION_FILES}
        DESTINATION translations)



# Custom target to generate translations
add_custom_target(generate_translations
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/generate_translations.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating translation files"
)