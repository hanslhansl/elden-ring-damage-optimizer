CPMAddPackage(
    NAME Catch2
    GITHUB_REPOSITORY catchorg/Catch2
    VERSION 3.10.0
)
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)


function(configure_test_executable target erdo_core_library test_prefix)
    # add an executable for the test
    add_executable(${target} tests.cpp)

    # link to the core library and Catch2
    target_link_libraries(${target} PRIVATE
        ${erdo_core_library}
        Catch2::Catch2WithMain
    )

    # copy xml_data to build directory
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/test_xml_data
            $<TARGET_FILE_DIR:${target}>/test_xml_data
    )

    catch_discover_tests(${target} TEST_PREFIX "${test_prefix}.")
endfunction()

# normal tests
configure_test_executable(tests erdo_core "normal")

# tests with coverage instrumentation
target_compile_options(erdo_core_tests PUBLIC
    -fprofile-instr-generate
    -fcoverage-mapping
)
target_link_options(erdo_core_tests PUBLIC
    -fprofile-instr-generate
)
configure_test_executable(test_with_coverage erdo_core_tests "coverage")

# tests with benchmarks
configure_test_executable(test_with_benchmarks erdo_core "benchmarks")
target_compile_definitions(test_with_benchmarks PRIVATE ENABLE_BENCHMARKS)
