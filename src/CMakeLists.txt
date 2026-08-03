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