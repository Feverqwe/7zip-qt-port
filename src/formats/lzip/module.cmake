add_library(sevenzip_lzlib_core STATIC src/formats/lzip/lzipio.c ${SEVENZIP_DEPENDENCY_DIR}/lzlib/lzlib.c)
target_include_directories(sevenzip_lzlib_core PUBLIC src ${SEVENZIP_DEPENDENCY_DIR}/lzlib)
set_target_properties(sevenzip_lzlib_core PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 99 C_STANDARD_REQUIRED ON)

add_library(sevenzip_lzip MODULE
    src/formats/lzip/lziphandler.cpp src/formats/lzip/lzipstream.cpp src/codecs/brotli/brotlimodule.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/ArchiveExports.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Common/DummyOutStream.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/ProgressUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/StreamUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/PropId.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/PropVariant.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyWindows.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyString.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/IntToString.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/StringConvert.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/UTFConvert.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Alloc.c)
target_include_directories(sevenzip_lzip PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP
    ${SEVENZIP_DEPENDENCY_DIR}/lzlib)
set_target_properties(sevenzip_lzip PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 99 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_lzip PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_lzip PRIVATE src/codecs/brotli/brotlimodule.def)
    target_link_libraries(sevenzip_lzip PRIVATE oleaut32)
endif()

function(sevenzip_deploy_lzip)
    add_custom_target(sevenzip_lzip_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_lzip>"
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats/$<TARGET_FILE_NAME:sevenzip_lzip>"
        DEPENDS sevenzip_lzip VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_lzip_runtime)
endfunction()

if(BUILD_TESTING)
    qt_add_executable(lzip_tests tests/formats/lzip/lzip_tests.cpp src/formats/lzip/lzipstream.cpp)
    set_target_properties(lzip_tests PROPERTIES AUTOMOC ON C_STANDARD 99 C_STANDARD_REQUIRED ON)
    target_include_directories(lzip_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP ${SEVENZIP_DEPENDENCY_DIR}/lzlib)
    target_link_libraries(lzip_tests PRIVATE sevenzip_panel Qt6::Test)
    target_compile_definitions(lzip_tests PRIVATE
        "SEVENZIP_LZIP_MODULE=\"$<TARGET_FILE:sevenzip_lzip>\""
        "SEVENZIP_LZIP_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_LZIP_FIXTURES=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/lzip\"")
    add_dependencies(lzip_tests sevenzip_lzip_runtime)
    add_test(NAME lzip COMMAND lzip_tests -silent)
    set_tests_properties(lzip PROPERTIES TIMEOUT 60)
endif()
