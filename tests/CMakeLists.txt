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
add_executable(tests tests.cpp)
target_link_libraries(tests PRIVATE
    erdo_core_tests
    Catch2::Catch2WithMain
)
target_compile_definitions(tests PRIVATE NO_BENCHMARKS)

# build tests with benchmarks but without coverage instrumentation
add_executable(test_benchmarks tests.cpp)
target_link_libraries(test_benchmarks PRIVATE
    erdo_core
    Catch2::Catch2WithMain
)

# copy xml_data to build directory
add_custom_command(TARGET tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/test_xml_data
        $<TARGET_FILE_DIR:tests>/test_xml_data
)
add_custom_command(TARGET test_benchmarks POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/test_xml_data
        $<TARGET_FILE_DIR:test_benchmarks>/test_xml_data
)

catch_discover_tests(tests)
catch_discover_tests(test_benchmarks)