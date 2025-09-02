add_library(core_engine)

target_sources(core_engine PRIVATE
    engine.cpp
    iocp.cpp
)

target_include_directories(core_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
