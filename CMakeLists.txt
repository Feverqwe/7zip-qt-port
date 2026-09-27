cmake_minimum_required(VERSION 3.21)

project(sevenzip_desktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(sevenzip_desktop
    src/main.cpp
)

target_link_libraries(sevenzip_desktop PRIVATE Qt6::Widgets)

set_target_properties(sevenzip_desktop PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(CTest)

if(BUILD_TESTING)
    add_test(NAME application_starts COMMAND sevenzip_desktop --smoke-test)
    set_tests_properties(application_starts PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()
