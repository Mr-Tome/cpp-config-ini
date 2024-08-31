add_library(source_directory_lib STATIC
    config_reader.cpp
    config_reader.hpp
)

target_include_directories(source_directory_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})