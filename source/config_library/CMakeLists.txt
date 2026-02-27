add_library(source_directory_lib STATIC
	common/config_schema.cpp
	common/config_schema.hpp
	common/config_value.hpp
	common/config_value.cpp
	common/validation_rules.cpp
	common/validation_rules.hpp
	common/type_parser.hpp
	common/type_parser.cpp
	ini_config_reader/ini_config_reader.cpp
	ini_config_reader/ini_config_reader.hpp
	ini_config_reader/cli_config_reader.cpp
	ini_config_reader/cli_config_reader.hpp
)

target_include_directories(source_directory_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
