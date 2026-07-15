cmake_minimum_required(VERSION 4.3.0 FATAL_ERROR)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 26)

# enable C++23 modules support
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
set(CMAKE_CXX_MODULE_STD 1)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# create project
project(erdo LANGUAGES CXX)


# necessary for Qt
add_compile_options("$<$<CXX_COMPILER_ID:Clang>:-pthread>")
# enable clang/libc++ experimental support
add_compile_options("$<$<CXX_COMPILER_ID:Clang>:-fexperimental-library>")

# enable g++ module support
add_compile_options("$<$<CXX_COMPILER_ID:GNU>:-fmodules>")


# add_compile_options(-g -gcodeview)
# add_link_options(-fuse-ld=lld -g -Wl,--pdb=)

# download and include CPM.cmake
file(
    DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.8/CPM.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
    EXPECTED_HASH SHA256=78ba32abdf798bc616bab7c73aac32a17bbd7b06ad9e26a6add69de8f3ae4791
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

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

# add Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# set up core libraries
set(SRC_DIR "src")
function(configure_erdo_core_library target)
    add_library(${target} STATIC)

    target_sources(${target}
        PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${SRC_DIR} FILES
            ${SRC_DIR}/meta.ixx
            ${SRC_DIR}/witchy.ixx
            ${SRC_DIR}/xml.ixx
            ${SRC_DIR}/calculator.ixx
            ${SRC_DIR}/optimizer.ixx
            ${SRC_DIR}/erdo.ixx
        )

    target_link_libraries(${target} PRIVATE
        BS_thread_pool
        pugixml::pugixml
    )

    if (MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL GNU)
        target_link_libraries(${target} PRIVATE stdc++exp) 
    endif()

    # copy xml_data to build directory
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/xml_data
            $<TARGET_FILE_DIR:${target}>/xml_data
    )
endfunction()
configure_erdo_core_library(erdo_core)
configure_erdo_core_library(erdo_core_tests)


# set up main executable
# qt_standard_project_setup()
add_executable(erdo
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/main_window.ui

)
target_sources(erdo PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${SRC_DIR} FILES ${SRC_DIR}/ui.ixx)
# qt_add_executable(erdo ${SRC_DIR}/main.cpp)
target_link_libraries(erdo PRIVATE
    erdo_core
    Qt6::Core
    Qt6::Widgets
)

# add testing
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()