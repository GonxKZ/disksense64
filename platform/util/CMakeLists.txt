add_library(platform_util)

target_sources(platform_util PRIVATE
    util.cpp
    trash.cpp
)

if(WIN32)
    target_link_libraries(platform_util PRIVATE ole32 shell32 propsys)
endif()
