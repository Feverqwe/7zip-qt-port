# Linux C framing/decoders stay in an isolated GPL module.
add_library(sevenzip_linux_frames_core STATIC
    src/lib/decompress_unlz4.c src/lib/decompress_unlzo.c
    src/lib/lz4/lz4_decompress.c src/lib/lzo/lzo1x_decompress_safe.c
    src/lib/crc/crc32-main.c)
target_include_directories(sevenzip_linux_frames_core PRIVATE src third_party/linux/include)
set_target_properties(sevenzip_linux_frames_core PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 11 C_STANDARD_REQUIRED ON)
add_library(sevenzip_linux_frames MODULE
    src/CPP/7zip/Archive/LinuxFrameHandler.cpp src/linuxframestream.cpp src/exfatmodule.cpp
    third_party/7zip/CPP/7zip/Archive/ArchiveExports.cpp
    third_party/7zip/CPP/7zip/Archive/Common/DummyOutStream.cpp
    third_party/7zip/CPP/7zip/Archive/Common/FindSignature.cpp
    third_party/7zip/CPP/7zip/Common/ProgressUtils.cpp
    third_party/7zip/CPP/7zip/Common/StreamUtils.cpp
    third_party/7zip/CPP/7zip/Common/PropId.cpp
    third_party/7zip/CPP/Windows/PropVariant.cpp
    third_party/7zip/CPP/Common/MyWindows.cpp
    third_party/7zip/CPP/Common/MyString.cpp
    third_party/7zip/CPP/Common/IntToString.cpp
    third_party/7zip/CPP/Common/StringConvert.cpp
    third_party/7zip/CPP/Common/UTFConvert.cpp)
target_include_directories(sevenzip_linux_frames PRIVATE src third_party/7zip/CPP)
target_link_libraries(sevenzip_linux_frames PRIVATE sevenzip_linux_frames_core)
set_target_properties(sevenzip_linux_frames PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX "")
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_linux_frames PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_linux_frames PRIVATE src/exfatmodule.def)
    target_link_libraries(sevenzip_linux_frames PRIVATE oleaut32)
endif()
function(sevenzip_deploy_linux_frames)
    add_custom_target(sevenzip_linux_frames_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_linux_frames>"
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats/$<TARGET_FILE_NAME:sevenzip_linux_frames>"
        DEPENDS sevenzip_linux_frames VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_linux_frames_runtime)
endfunction()
