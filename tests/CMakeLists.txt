CPMAddPackage(
    NAME Catch2
    GITHUB_REPOSITORY catchorg/Catch2
    VERSION 3.10.0
)
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)

option(ERDO_TEST_COVERAGE "Instrument the core and tests for LLVM coverage (Clang)" OFF)
option(ERDO_TEST_BENCHMARKS "Enable Catch2 benchmark sections in the tests" OFF)

# Local game XML is optional; the portable importer checks always run.
set(ERDO_MOVESET_XML_ROOT "" CACHE PATH "Directory containing unpacked moveset XML")
set(ERDO_MOVESET_BEHAVIOR_XML "" CACHE FILEPATH "Optional HKLib XML export of the player behavior graph")

add_executable(tests tests.cpp moveset_tests.cpp move_calculation_tests.cpp behavior_graph_tests.cpp)
target_link_libraries(tests PRIVATE erdo_core Catch2::Catch2WithMain)

# QTemporaryDir in behavior_graph_tests.cpp needs QtCore.
find_package(Qt6 REQUIRED COMPONENTS Core)
target_link_libraries(tests PRIVATE Qt6::Core)
set(test_dll_paths "$<TARGET_FILE_DIR:Qt6::Core>")

if(ERDO_MOVESET_XML_ROOT)
    file(TO_CMAKE_PATH "${ERDO_MOVESET_XML_ROOT}" moveset_xml_root)
    target_compile_definitions(tests PRIVATE ERDO_MOVESET_XML_ROOT="${moveset_xml_root}")
endif()

if(ERDO_MOVESET_BEHAVIOR_XML)
    file(TO_CMAKE_PATH "${ERDO_MOVESET_BEHAVIOR_XML}" moveset_behavior_xml)
    target_compile_definitions(tests PRIVATE ERDO_MOVESET_BEHAVIOR_XML="${moveset_behavior_xml}")
endif()

set(test_properties)
if(ERDO_TEST_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|AppleClang)$")
        message(FATAL_ERROR "ERDO_TEST_COVERAGE requires Clang/LLVM coverage support.")
    endif()

    # Reuse the core library; this build's consumers also need coverage flags
    # and the profiling runtime. No second core library is compiled.
    target_compile_options(erdo_core PUBLIC -fprofile-instr-generate -fcoverage-mapping)
    target_link_options(erdo_core PUBLIC -fprofile-instr-generate)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/profiles")
    # CTest runs each case in its own process, possibly in parallel.
    set(test_properties ENVIRONMENT "LLVM_PROFILE_FILE=${CMAKE_CURRENT_BINARY_DIR}/profiles/erdo-%p.profraw")
endif()

if(ERDO_TEST_BENCHMARKS)
    target_compile_definitions(tests PRIVATE ENABLE_BENCHMARKS)
endif()

copy_game_data(tests)
catch_discover_tests(tests
    TEST_PREFIX "normal."
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:tests>"
    DL_PATHS ${test_dll_paths}
    PROPERTIES ${test_properties}
)
