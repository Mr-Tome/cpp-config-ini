add_library(source_directory_lib STATIC
	common/config_schema.cpp
	common/config_schema.hpp
	common/config_value.hpp
	common/config_value.cpp
	common/validation_rules.cpp
	common/validation_rules.hpp
	common/type_parser.hpp
	common/type_parser.cpp
	
	common/schema_evolver.hpp
	common/schema_evolver.cpp
	common/schema_migration.hpp
	common/schema_migration.cpp

	config_reader/ini_config_reader.cpp
	config_reader/ini_config_reader.hpp
	config_reader/cli_config_reader.cpp
	config_reader/cli_config_reader.hpp
	config_reader/cli_parser.cpp
	config_reader/cli_parser.hpp
	config_reader/config_reader.hpp
	config_reader/config_reader_base.hpp
	config_reader/config_reader_base.cpp
	
	config_reader/PersistenceReaders/IPersistenceReader.hpp
)

target_include_directories(source_directory_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(source_directory_lib PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic 
        -Wshadow 
        -Wconversion 
        -Wnull-dereference 
        -Werror
        -Wcast-align 
        -Wunreachable-code 
        -Wdeprecated 
        -Wfloat-equal 
        -Wformat=2 
        -Wlogical-op 
        -Woverloaded-virtual 
        -Wextra-semi 
        #-fsanitize=undefined 
        #-fsanitize=address 
)
