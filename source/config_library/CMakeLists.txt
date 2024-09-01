add_library(source_directory_lib STATIC
    config_reader.cpp
    config_reader.hpp
	validation_rules.cpp
    validation_rules.hpp
)

target_include_directories(source_directory_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})