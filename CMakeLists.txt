cmake_minimum_required(VERSION 4.3.0 FATAL_ERROR)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 26)

# enable C++23 modules support
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
set(CMAKE_CXX_MODULE_STD 1)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# create project
project(elden-ring-damage-optimizer LANGUAGES CXX)

# download CPM.cmake
file(
    DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.8/CPM.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
    EXPECTED_HASH SHA256=78ba32abdf798bc616bab7c73aac32a17bbd7b06ad9e26a6add69de8f3ae4791
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

# add nlohmann json
add_compile_definitions(JSON_USE_IMPLICIT_CONVERSIONS=0)
CPMAddPackage(
    NAME nlohmann_json
    GITHUB_REPOSITORY nlohmann/json
    GIT_TAG develop
)

# add thread pool
add_compile_definitions(BS_THREAD_POOL_IMPORT_STD)
CPMAddPackage(
    NAME BS_thread_pool
    GITHUB_REPOSITORY bshoshany/thread-pool
    VERSION 5.1.0
    EXCLUDE_FROM_ALL
    SYSTEM
)
add_library(BS_thread_pool)
target_sources(BS_thread_pool PUBLIC FILE_SET CXX_MODULES FILES ${BS_thread_pool_SOURCE_DIR}/modules/BS.thread_pool.cppm)
target_include_directories(BS_thread_pool PRIVATE ${BS_thread_pool_SOURCE_DIR}/include)

# add pugixml
add_compile_definitions(PUGIXML_HEADER_ONLY)
CPMAddPackage(
    NAME pugixml
    GITHUB_REPOSITORY zeux/pugixml
    VERSION 1.15
)

# add testing
include(CTest)
add_subdirectory(tests)

set(SRC_DIR "src")

# set up core library
add_library(elden_ring_damage_optimizer_core)
target_sources(elden_ring_damage_optimizer_core 
    PUBLIC FILE_SET CXX_MODULES
        BASE_DIRS ${SRC_DIR}
        FILES
        ${SRC_DIR}/meta.ixx
        ${SRC_DIR}/witchy.ixx
        ${SRC_DIR}/calculator.ixx
        ${SRC_DIR}/xml.ixx
        ${SRC_DIR}/erdo.ixx
        # ${SRC_DIR}/ui.ixx
)
target_link_libraries(elden_ring_damage_optimizer_core PRIVATE
    # nlohmann_json_modules
    nlohmann_json::nlohmann_json
    BS_thread_pool
    pugixml::pugixml
)

if (MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_link_libraries(elden_ring_damage_optimizer_core PRIVATE stdc++exp) # std:: print, println, ...
endif()

add_custom_command(TARGET elden_ring_damage_optimizer_core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/xml_data
        $<TARGET_FILE_DIR:elden_ring_damage_optimizer_core>/xml_data
)


# set up main executable
add_executable(elden_ring_damage_optimizer ${SRC_DIR}/main.cpp)
target_link_libraries(elden_ring_damage_optimizer PRIVATE
    elden_ring_damage_optimizer_core
)