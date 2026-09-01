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

# ASan and UBSan for debug builds
add_compile_options($<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-fsanitize=address,undefined>)
add_compile_options($<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-fno-omit-frame-pointer>)
add_link_options($<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-fsanitize=address,undefined>)


# add_compile_options(-g -gcodeview)
# add_link_options(-fuse-ld=lld -g -Wl,--pdb=)


# download and include CPM.cmake
# Define CPM_SOURCE_CACHE (before include CPM) to fix patch bug in CPM
if(NOT DEFINED ENV{CPM_SOURCE_CACHE})
    set(ENV{CPM_SOURCE_CACHE} ${CMAKE_CURRENT_BINARY_DIR}/.cpm-cache)
endif()
file(
    DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/CPM.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)


# add core library
add_subdirectory(src/core)

# add ui
add_subdirectory(src/app)

# add testing
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()