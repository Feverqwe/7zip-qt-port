cmake_minimum_required(VERSION 3.21)

project(sevenzip_desktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(sevenzip_desktop
    src/main.cpp
    src/mainwindow.h
    src/archivebackend.cpp
    src/archivebackend.h
)

target_link_libraries(sevenzip_desktop PRIVATE Qt6::Widgets)

set_target_properties(sevenzip_desktop PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(CTest)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(archive_backend_tests tests/archive_backend_tests.cpp src/archivebackend.cpp src/archivebackend.h)
    target_include_directories(archive_backend_tests PRIVATE src)
    target_link_libraries(archive_backend_tests PRIVATE Qt6::Test)
    qt_add_executable(ui_layout_tests tests/ui_layout_tests.cpp src/archivebackend.cpp src/archivebackend.h src/mainwindow.h)
    target_include_directories(ui_layout_tests PRIVATE src)
    target_link_libraries(ui_layout_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME ui_layout COMMAND ui_layout_tests)
    set_tests_properties(ui_layout PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SCREENSHOT_DIR=${CMAKE_BINARY_DIR}/ui-screenshots"
    )
    add_test(NAME archive_backend COMMAND archive_backend_tests)
    add_test(NAME application_starts COMMAND sevenzip_desktop --smoke-test)
    set_tests_properties(application_starts PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()
