cmake_minimum_required(VERSION 3.21)

project(FukeBoard
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.3 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(FukeBoard
    main.cpp
    mainwindow.cpp
    canvaswidget.cpp
    canvasitem.cpp
    canvascommand.cpp
    canvasfile.cpp
    canvasdocument.cpp
    selectioncontroller.cpp
    strokeitem.cpp
    textboxitem.cpp
    imageitem.cpp
    milegriditem.cpp
    fukearchive.cpp
    resources.qrc
)

target_link_libraries(FukeBoard
    PRIVATE
        Qt6::Widgets
)

set_target_properties(FukeBoard PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

include(CTest)

if(BUILD_TESTING)
    find_package(Qt6 6.3 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
