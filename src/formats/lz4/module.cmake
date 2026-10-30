add_library(sevenzip_lz4_frame MODULE
    src/formats/lz4/Lz4FrameHandler.cpp src/formats/lz4/lz4framestream.cpp src/codecs/brotli/brotlimodule.cpp
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
target_include_directories(sevenzip_lz4_frame PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP
    ${SEVENZIP_DEPENDENCY_DIR}/lz4/lib)
set_target_properties(sevenzip_lz4_frame PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 99 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_lz4_frame PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_lz4_frame PRIVATE src/codecs/brotli/brotlimodule.def)
    target_link_libraries(sevenzip_lz4_frame PRIVATE oleaut32)
endif()

function(sevenzip_deploy_lz4_frame)
    add_custom_target(sevenzip_lz4_frame_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_lz4_frame>"
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats/$<TARGET_FILE_NAME:sevenzip_lz4_frame>"
        DEPENDS sevenzip_lz4_frame VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_lz4_frame_runtime)
endfunction()

if(BUILD_TESTING)
    qt_add_executable(lz4_frame_tests tests/formats/lz4/lz4_frame_tests.cpp src/formats/lz4/lz4framestream.cpp)
    set_target_properties(lz4_frame_tests PROPERTIES AUTOMOC ON)
    target_include_directories(lz4_frame_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP ${SEVENZIP_DEPENDENCY_DIR}/lz4/lib)
    target_link_libraries(lz4_frame_tests PRIVATE sevenzip_panel Qt6::Test)
    target_compile_definitions(lz4_frame_tests PRIVATE
        "SEVENZIP_LZ4_MODULE=\"$<TARGET_FILE:sevenzip_lz4_frame>\""
        "SEVENZIP_LZ4_LEGACY_MODULE=\"$<TARGET_FILE:sevenzip_linux_frames>\""
        "SEVENZIP_LZ4_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\"")
    add_dependencies(lz4_frame_tests sevenzip_lz4_frame_runtime sevenzip_linux_frames_runtime)
    add_test(NAME lz4_frame COMMAND lz4_frame_tests -silent)
    set_tests_properties(lz4_frame PROPERTIES TIMEOUT 60)
endif()
