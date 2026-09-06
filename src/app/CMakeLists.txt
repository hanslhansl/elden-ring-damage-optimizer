# add Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Concurrent PrintSupport)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# add KDChart
CPMAddPackage(
    NAME KDChart
    GITHUB_REPOSITORY KDAB/KDChart
    GIT_TAG kdchart-3.0.0
    OPTIONS
        "KDChart_QT6 ON"
        "KDChart_EXAMPLES OFF"
        "KDChart_TESTS OFF"
        "KDChart_DOCS OFF"

    PATCHES
        ${CMAKE_CURRENT_SOURCE_DIR}/patches/kdchart-normalplotter-nan.patch
)
add_library(KDChart::kdchart ALIAS kdchart)


add_library(erdo_ui STATIC)
target_sources(erdo_ui PUBLIC FILE_SET CXX_MODULES FILES
    settings.ixx
    weapons_table.ixx
    plot_tab.ixx
    ui.ixx
)
qt_add_ui(erdo_ui SOURCES
    main_window.ui
    stats_tab.ui
    optimize_widget.ui
)
target_link_libraries(erdo_ui PUBLIC#PRIVATE
    erdo_core
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Concurrent
    Qt6::PrintSupport
    KDChart::kdchart
)

# set up main executable
add_executable(erdo
    main.cpp
)
set_target_properties(erdo PROPERTIES OUTPUT_NAME "elden-ring-damage-optimizer")
target_link_libraries(erdo PRIVATE
    erdo_ui
)
copy_xml_data(erdo)

install(TARGETS erdo
    DESTINATION .
)

# copy readme and license files
install(FILES ${CMAKE_SOURCE_DIR}/README.md ${CMAKE_SOURCE_DIR}/LICENSE ${CMAKE_SOURCE_DIR}/THIRD-PARTY-NOTICES.txt
    TYPE BIN
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/LICENSES
    TYPE BIN
)

# use qt deploy script to copy dependencies (e.g. DLLs) to the install directory
qt_generate_deploy_app_script(
    TARGET erdo
    OUTPUT_SCRIPT deploy_script
)
install(SCRIPT ${deploy_script})

# copy binaries to root of install directory
install(CODE [[
    set(_root "${CMAKE_INSTALL_PREFIX}")
    set(bin_dir "${_root}/bin")

    if(EXISTS "${bin_dir}")
        file(GLOB bin_files "${bin_dir}/*")

        foreach(file IN LISTS bin_files)
            get_filename_component(name "${file}" NAME)
            set(dst "${_root}/${name}")

            # file(RENAME) does not reliably replace an existing
            # destination on Windows.
            if(EXISTS "${dst}")
                file(REMOVE_RECURSE "${dst}")
            endif()

            file(RENAME
                "${file}"
                "${dst}"
            )
        endforeach()

        file(REMOVE_RECURSE "${bin_dir}")
    endif()

    file(REMOVE_RECURSE "${_bin}")

    # Fix qt.conf for the new location.
    set(_qt_conf "${_root}/qt.conf")

    if(EXISTS "${_qt_conf}")
        file(READ "${_qt_conf}" _qt_conf_content)

        string(REPLACE
            "Prefix = .."
            "Prefix = ."
            _qt_conf_content
            "${_qt_conf_content}"
        )

        file(WRITE
            "${_qt_conf}"
            "${_qt_conf_content}"
        )
    endif()
]])

# set up packaging
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "elden-ring-damage-optimizer")
set(CPACK_PACKAGE_VERSION "4.1.1")
include(CPack)