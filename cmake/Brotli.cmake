# Decoder sources retain Google's C language and source layout. No encoder is built.
file(GLOB brotli_common_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/brotli/c/common/*.c")
file(GLOB brotli_decoder_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/brotli/c/dec/*.c")
add_library(sevenzip_brotli_core STATIC ${brotli_common_sources} ${brotli_decoder_sources})
target_include_directories(sevenzip_brotli_core PUBLIC third_party/brotli/c/include)
set_target_properties(sevenzip_brotli_core PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 99 C_STANDARD_REQUIRED ON)
add_library(sevenzip_brotli MODULE
    src/CPP/7zip/Archive/BrotliHandler.cpp
    src/CPP/7zip/Compress/BrotliDecoder.cpp src/brotlimodule.cpp
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
target_include_directories(sevenzip_brotli PRIVATE src third_party/7zip/CPP
    third_party/brotli/c/include)
set_target_properties(sevenzip_brotli PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 99 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_brotli PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_brotli PRIVATE src/brotlimodule.def)
    target_link_libraries(sevenzip_brotli PRIVATE oleaut32)
endif()

function(sevenzip_deploy_brotli)
    add_custom_target(sevenzip_brotli_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_brotli>"
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats/$<TARGET_FILE_NAME:sevenzip_brotli>"
        DEPENDS sevenzip_brotli VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_brotli_runtime)
endfunction()

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(brotli_tests tests/brotli_tests.cpp)
    set_target_properties(brotli_tests PROPERTIES AUTOMOC ON)
    target_include_directories(brotli_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(brotli_tests PRIVATE sevenzip_panel Qt6::Test)
    target_compile_definitions(brotli_tests PRIVATE
        "SEVENZIP_BROTLI_MODULE=\"$<TARGET_FILE:sevenzip_brotli>\""
        "SEVENZIP_BROTLI_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_BROTLI_FIXTURES=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/brotli\"")
    add_dependencies(brotli_tests sevenzip_brotli_runtime)
    add_test(NAME brotli COMMAND brotli_tests -silent)
    set_tests_properties(brotli PROPERTIES TIMEOUT 60)
endif()
