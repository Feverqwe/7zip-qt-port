cmake_minimum_required(VERSION 3.21)

project(sevenzip_desktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(sevenzip_desktop
    src/main.cpp
    src/mainwindow.h
    src/createarchivedialog.h
    src/combodialog.h
    src/copydialog.h
    src/extractdialog.h
    src/archivebackend.cpp
    src/archivebackend.h
    src/filesystemscanner.cpp
    src/filesystemscanner.h
    src/fileviewerdialog.cpp
    src/fileviewerdialog.h
    src/settingsdialog.cpp
    src/settingsdialog.h
    src/propertiesdialog.cpp
    src/propertiesdialog.h
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
    qt_add_executable(ui_layout_tests tests/ui_layout_tests.cpp src/archivebackend.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/mainwindow.h src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.cpp src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h)
    target_include_directories(ui_layout_tests PRIVATE src)
    target_link_libraries(ui_layout_tests PRIVATE Qt6::Widgets Qt6::Test)
    qt_add_executable(ui_state_tests tests/ui_state_tests.cpp src/archivebackend.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/mainwindow.h src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.cpp src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h)
    target_include_directories(ui_state_tests PRIVATE src)
    target_link_libraries(ui_state_tests PRIVATE Qt6::Widgets Qt6::Test)
    qt_add_executable(flat_view_tests tests/flat_view_tests.cpp src/archivebackend.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/mainwindow.h src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.cpp src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h)
    qt_add_executable(settings_tests tests/settings_tests.cpp src/archivebackend.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/mainwindow.h src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.cpp src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h)
    target_include_directories(settings_tests PRIVATE src)
    target_link_libraries(settings_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME settings COMMAND settings_tests)
    set_tests_properties(settings PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/settings-tests.ini")
    qt_add_executable(properties_tests tests/properties_tests.cpp src/archivebackend.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/mainwindow.h src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.cpp src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h)
    target_include_directories(properties_tests PRIVATE src)
    target_link_libraries(properties_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME properties COMMAND properties_tests)
    set_tests_properties(properties PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/properties-tests.ini")
    target_include_directories(flat_view_tests PRIVATE src)
    target_link_libraries(flat_view_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME flat_view COMMAND flat_view_tests)
    set_tests_properties(flat_view PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/flat-view-settings.ini"
    )
    add_test(NAME ui_state COMMAND ui_state_tests)
    set_tests_properties(ui_state PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SCREENSHOT_DIR=${CMAKE_BINARY_DIR}/ui-screenshots;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/ui-state-settings.ini"
    )
    add_test(NAME ui_layout COMMAND ui_layout_tests)
    set_tests_properties(ui_layout PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SCREENSHOT_DIR=${CMAKE_BINARY_DIR}/ui-screenshots;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/ui-layout-settings.ini"
    )
    add_test(NAME archive_backend COMMAND archive_backend_tests)
    qt_add_executable(extraction_options_tests tests/extraction_options_tests.cpp src/archivebackend.cpp src/archivebackend.h)
    target_include_directories(extraction_options_tests PRIVATE src)
    target_link_libraries(extraction_options_tests PRIVATE Qt6::Test)
    add_test(NAME extraction_options COMMAND extraction_options_tests)
    qt_add_executable(compression_tests tests/compression_tests.cpp src/archivebackend.cpp src/archivebackend.h src/createarchivedialog.h)
    target_include_directories(compression_tests PRIVATE src)
    target_link_libraries(compression_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME compression COMMAND compression_tests)
    set_tests_properties(compression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(file_viewer_tests tests/file_viewer_tests.cpp src/archivebackend.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/mainwindow.h src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.cpp src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h)
    target_include_directories(file_viewer_tests PRIVATE src)
    target_link_libraries(file_viewer_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME file_viewer COMMAND file_viewer_tests)
    set_tests_properties(file_viewer PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/file-viewer-settings.ini")
    add_test(NAME application_starts COMMAND sevenzip_desktop --smoke-test)
    set_tests_properties(application_starts PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()
