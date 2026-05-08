set(TEST_COMPILE_OPTIONS
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
    -Wformat=2
    -Wlogical-op
    -Woverloaded-virtual
    -Wextra-semi
)

function(add_config_test TEST_NAME SOURCE_FILE)
    add_executable(${TEST_NAME} ${SOURCE_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE source_directory_lib)
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/source
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_compile_options(${TEST_NAME} PRIVATE ${TEST_COMPILE_OPTIONS})
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

add_config_test(test_validation_rules  test_validation_rules.cpp)
add_config_test(test_config_item       test_config_item.cpp)
add_config_test(test_config_schema     test_config_schema.cpp)
add_config_test(test_type_parser       test_type_parser.cpp)
add_config_test(test_schema_evolver    test_schema_evolver.cpp)
add_config_test(test_schema_migration  test_schema_migration.cpp)
add_config_test(test_ini_config_reader test_ini_config_reader.cpp)
add_config_test(test_custom_type       test_custom_type.cpp)
add_config_test(test_cli_config_reader test_cli_config_reader.cpp)
