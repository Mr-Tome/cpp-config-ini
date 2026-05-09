cmake_minimum_required(VERSION 3.20)
project(configs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the config_library directory
add_subdirectory(source/config_library)

enable_testing()
add_subdirectory(tests)

# Add the executable
add_executable(configs source/main.cpp)

# Link the config_library to the main executable
target_link_libraries(configs PRIVATE source_directory_lib)

# Include the config_library headers
target_include_directories(configs PRIVATE source)

target_compile_options(configs PRIVATE 
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

# Includes examples down in the example directory. Assumes that each .cpp
# has a standalone main.
file(GLOB_RECURSE EXAMPLE_SOURCES CONFIGURE_DEPENDS "examples/*.cpp")

foreach(EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
	get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)
	add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCE})
	target_link_libraries(${EXAMPLE_NAME} PRIVATE source_directory_lib)
	target_include_directories(${EXAMPLE_NAME} PRIVATE source)
	target_compile_options(${EXAMPLE_NAME} PRIVATE 
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
endforeach()
