# add Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Concurrent)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_library(erdo_ui STATIC)
target_sources(erdo_ui PUBLIC FILE_SET CXX_MODULES FILES ui.ixx weapons_table.ixx)
qt_add_ui(erdo_ui SOURCES main_window.ui)
target_link_libraries(erdo_ui PRIVATE
    erdo_core
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Concurrent
)

# set up main executable
add_executable(erdo
    main.cpp
)
target_link_libraries(erdo PRIVATE
    erdo_ui
)
copy_xml_data(erdo)