CPMAddPackage(
    NAME Catch2
    GITHUB_REPOSITORY catchorg/Catch2
    VERSION 3.10.0
)
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)

# build tests with coverage instrumentation but without benchmarks
target_compile_options(erdo_core_tests PUBLIC
    -fprofile-instr-generate
    -fcoverage-mapping
)
target_link_options(erdo_core_tests PUBLIC
    -fprofile-instr-generate
)
add_executable(test_with_coverage tests.cpp)
target_link_libraries(test_with_coverage PRIVATE
    erdo_core_tests
    Catch2::Catch2WithMain
)
target_compile_definitions(test_with_coverage PRIVATE NO_BENCHMARKS)

# build tests with benchmarks but without coverage instrumentation
add_executable(test_with_benchmarks tests.cpp)
target_link_libraries(test_with_benchmarks PRIVATE
    erdo_core
    Catch2::Catch2WithMain
)

# copy xml_data to build directory
add_custom_command(TARGET test_with_coverage POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/test_xml_data
        $<TARGET_FILE_DIR:test_with_coverage>/test_xml_data
)
add_custom_command(TARGET test_with_benchmarks POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/test_xml_data
        $<TARGET_FILE_DIR:test_with_benchmarks>/test_xml_data
)

catch_discover_tests(test_with_coverage)
catch_discover_tests(test_with_benchmarks)