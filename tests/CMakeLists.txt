add_library(fukeboard_test_support STATIC
    ${PROJECT_SOURCE_DIR}/canvasdocument.cpp
    ${PROJECT_SOURCE_DIR}/canvasfile.cpp
    ${PROJECT_SOURCE_DIR}/canvasitem.cpp
    ${PROJECT_SOURCE_DIR}/fukearchive.cpp
    ${PROJECT_SOURCE_DIR}/imageitem.cpp
    ${PROJECT_SOURCE_DIR}/milegriditem.cpp
    ${PROJECT_SOURCE_DIR}/strokeitem.cpp
    ${PROJECT_SOURCE_DIR}/textboxitem.cpp
)

target_include_directories(fukeboard_test_support
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(fukeboard_test_support
    PRIVATE
        Qt6::Widgets
)

qt_add_executable(tst_fukearchive
    tst_fukearchive.cpp
)

target_include_directories(tst_fukearchive
    PRIVATE
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(tst_fukearchive
    PRIVATE
        Qt6::Test
        fukeboard_test_support
)

add_test(
    NAME FukeArchive
    COMMAND tst_fukearchive
)

qt_add_executable(tst_canvas_components
    tst_canvas_components.cpp
)

target_link_libraries(tst_canvas_components
    PRIVATE
        Qt6::Test
        Qt6::Widgets
        fukeboard_test_support
)

add_test(
    NAME CanvasComponents
    COMMAND tst_canvas_components
)

set_tests_properties(CanvasComponents PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_add_executable(tst_canvaswidget
    tst_canvaswidget.cpp
    ${PROJECT_SOURCE_DIR}/canvascommand.cpp
    ${PROJECT_SOURCE_DIR}/canvaswidget.cpp
    ${PROJECT_SOURCE_DIR}/selectioncontroller.cpp
)

target_link_libraries(tst_canvaswidget
    PRIVATE
        Qt6::Test
        Qt6::Widgets
        fukeboard_test_support
)

add_test(
    NAME CanvasWidget
    COMMAND tst_canvaswidget
)

set_tests_properties(CanvasWidget PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
