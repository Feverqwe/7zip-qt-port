# Charset tables and converters are provided by the host operating system.
add_library(sevenzip_nls_system STATIC src/fs/nls/system.c src/fs/nls/registry.c)
target_include_directories(sevenzip_nls_system PRIVATE src)
set_target_properties(sevenzip_nls_system PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 11 C_STANDARD_REQUIRED ON
    C_VISIBILITY_PRESET hidden)
if(NOT WIN32)
    find_package(Iconv REQUIRED)
    target_link_libraries(sevenzip_nls_system PUBLIC Iconv::Iconv)
endif()
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(filesystem_charset_tests tests/filesystem_charset_tests.cpp
        src/fs/fat/dir.c src/fs/nls/fatnames.c src/fs/nls/nls_base.c)
    set_target_properties(filesystem_charset_tests PROPERTIES AUTOMOC ON C_STANDARD 11)
    target_include_directories(filesystem_charset_tests PRIVATE src)
    target_link_libraries(filesystem_charset_tests PRIVATE sevenzip_nls_system Qt6::Test)
    add_test(NAME filesystem_charset COMMAND filesystem_charset_tests -silent)
    set_tests_properties(filesystem_charset PROPERTIES TIMEOUT 30)
endif()
