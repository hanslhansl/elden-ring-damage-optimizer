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
target_sources(BS_thread_pool PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${BS_thread_pool_SOURCE_DIR}/modules FILES ${BS_thread_pool_SOURCE_DIR}/modules/BS.thread_pool.cppm)
target_include_directories(BS_thread_pool PRIVATE ${BS_thread_pool_SOURCE_DIR}/include)

# add pugixml
CPMAddPackage(
    NAME pugixml
    GITHUB_REPOSITORY zeux/pugixml
    VERSION 1.15
)

# set up core libraries
function(configure_erdo_core_library target)
    add_library(${target} STATIC)

    target_compile_definitions(${target} PRIVATE PUGIXML_HEADER_ONLY)

    target_sources(${target}
        PUBLIC FILE_SET CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_FUNCTION_LIST_DIR}
        FILES
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/meta.ixx
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/witchy.ixx
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/xml.ixx
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/parser.ixx
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/calculator.ixx
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/optimizer.ixx
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/erdo.ixx
        )

    target_link_libraries(${target} PUBLIC
        BS_thread_pool
        pugixml::pugixml
    )

    if (MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL GNU)
        target_link_libraries(${target} PRIVATE stdc++exp) 
    endif()
endfunction()
configure_erdo_core_library(erdo_core)

function(copy_xml_data target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/xml_data
            $<TARGET_FILE_DIR:${target}>/xml_data
        COMMENT "Copying XML runtime data for ${target}"
    )
endfunction()