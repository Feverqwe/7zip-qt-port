add_library(sevenzip_lzlib_core STATIC src/lzipio.c third_party/lzlib/lzlib.c)
target_include_directories(sevenzip_lzlib_core PUBLIC src third_party/lzlib)
set_target_properties(sevenzip_lzlib_core PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 99 C_STANDARD_REQUIRED ON)

add_library(sevenzip_lzip MODULE
    src/lziphandler.cpp src/lzipstream.cpp src/brotlimodule.cpp
    third_party/7zip/CPP/7zip/Archive/ArchiveExports.cpp
    third_party/7zip/CPP/7zip/Archive/Common/DummyOutStream.cpp
    third_party/7zip/CPP/7zip/Common/ProgressUtils.cpp
    third_party/7zip/CPP/7zip/Common/StreamUtils.cpp
    third_party/7zip/CPP/7zip/Common/PropId.cpp
    third_party/7zip/CPP/Windows/PropVariant.cpp
    third_party/7zip/CPP/Common/MyWindows.cpp
    third_party/7zip/CPP/Common/MyString.cpp
    third_party/7zip/CPP/Common/IntToString.cpp
    third_party/7zip/CPP/Common/StringConvert.cpp
    third_party/7zip/CPP/Common/UTFConvert.cpp
    third_party/7zip/C/Alloc.c)
target_include_directories(sevenzip_lzip PRIVATE src third_party/7zip/CPP
    third_party/lzlib)
set_target_properties(sevenzip_lzip PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 99 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_lzip PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_lzip PRIVATE src/brotlimodule.def)
    target_link_libraries(sevenzip_lzip PRIVATE oleaut32)
endif()

function(sevenzip_deploy_lzip)
    add_custom_target(sevenzip_lzip_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_lzip>"
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats/$<TARGET_FILE_NAME:sevenzip_lzip>"
        DEPENDS sevenzip_lzip VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_lzip_runtime)
endfunction()

if(BUILD_TESTING)
    qt_add_executable(lzip_tests tests/lzip_tests.cpp src/lzipstream.cpp)
    set_target_properties(lzip_tests PROPERTIES AUTOMOC ON C_STANDARD 99 C_STANDARD_REQUIRED ON)
    target_include_directories(lzip_tests PRIVATE src third_party/7zip/CPP third_party/lzlib)
    target_link_libraries(lzip_tests PRIVATE sevenzip_panel Qt6::Test)
    target_compile_definitions(lzip_tests PRIVATE
        "SEVENZIP_LZIP_MODULE=\"$<TARGET_FILE:sevenzip_lzip>\""
        "SEVENZIP_LZIP_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_LZIP_FIXTURES=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/lzip\"")
    add_dependencies(lzip_tests sevenzip_lzip_runtime)
    add_test(NAME lzip COMMAND lzip_tests -silent)
    set_tests_properties(lzip PROPERTIES TIMEOUT 60)
endif()
