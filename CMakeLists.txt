cmake_minimum_required(VERSION 3.10)
project(configs)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the config_library directory
add_subdirectory(source/config_library)

# Add the executable
add_executable(configs source/main.cpp)

# Link the config_library to the main executable
target_link_libraries(configs PRIVATE source_directory_lib)

# Include the config_library headers
target_include_directories(configs PRIVATE source)