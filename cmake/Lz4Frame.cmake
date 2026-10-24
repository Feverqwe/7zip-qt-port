add_library(sevenzip_lz4_frame MODULE
    src/CPP/7zip/Archive/Lz4FrameHandler.cpp src/lz4framestream.cpp src/brotlimodule.cpp
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
target_link_libraries(sevenzip_lz4_frame PRIVATE sevenzip_lz4_frame_core)
target_include_directories(sevenzip_lz4_frame PRIVATE src third_party/7zip/CPP
    third_party/lz4/lib)
set_target_properties(sevenzip_lz4_frame PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 99 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_lz4_frame PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_lz4_frame PRIVATE src/brotlimodule.def)
    target_link_libraries(sevenzip_lz4_frame PRIVATE oleaut32)
endif()

function(sevenzip_deploy_lz4_frame)
    add_custom_target(sevenzip_lz4_frame_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_lz4_frame>"
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats/$<TARGET_FILE_NAME:sevenzip_lz4_frame>"
        DEPENDS sevenzip_lz4_frame VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_lz4_frame_runtime)
endfunction()

if(BUILD_TESTING)
    qt_add_executable(lz4_frame_tests tests/lz4_frame_tests.cpp src/lz4framestream.cpp)
    set_target_properties(lz4_frame_tests PROPERTIES AUTOMOC ON)
    target_include_directories(lz4_frame_tests PRIVATE src third_party/7zip/CPP third_party/lz4/lib)
    target_link_libraries(lz4_frame_tests PRIVATE sevenzip_panel Qt6::Test sevenzip_lz4_frame_core)
    target_compile_definitions(lz4_frame_tests PRIVATE
        "SEVENZIP_LZ4_MODULE=\"$<TARGET_FILE:sevenzip_lz4_frame>\""
        "SEVENZIP_LZ4_LEGACY_MODULE=\"$<TARGET_FILE:sevenzip_linux_frames>\""
        "SEVENZIP_LZ4_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\"")
    add_dependencies(lz4_frame_tests sevenzip_lz4_frame_runtime sevenzip_linux_frames_runtime)
    add_test(NAME lz4_frame COMMAND lz4_frame_tests -silent)
    set_tests_properties(lz4_frame PROPERTIES TIMEOUT 60)
endif()
