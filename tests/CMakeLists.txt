CPMAddPackage(
    NAME Catch2
    GITHUB_REPOSITORY catchorg/Catch2
    VERSION 3.10.0
)
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)

include(Catch)

add_executable(tests tests.cpp)

target_link_libraries(tests PRIVATE
    elden_ring_damage_optimizer_core
    Catch2::Catch2WithMain
)

# copy xml_data to build directory
add_custom_command(TARGET tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/test_xml_data
        $<TARGET_FILE_DIR:tests>/test_xml_data
)

catch_discover_tests(tests)