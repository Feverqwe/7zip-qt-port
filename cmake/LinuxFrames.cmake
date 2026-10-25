# Linux C framing/decoders stay in an isolated GPL module.
add_library(sevenzip_linux_frames_core STATIC
    src/lib/decompress_unlz4.c src/lib/decompress_unlzo.c
    src/lib/lzo/lzo1x_decompress_safe.c
    src/lib/crc/crc32-main.c)
target_include_directories(sevenzip_linux_frames_core PRIVATE src third_party/linux/include)
target_link_libraries(sevenzip_linux_frames_core PUBLIC sevenzip_lz4 sevenzip_zisofs_zlib)
set_target_properties(sevenzip_linux_frames_core PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 11 C_STANDARD_REQUIRED ON)
add_library(sevenzip_linux_frames MODULE
    src/CPP/7zip/Archive/LinuxFrameHandler.cpp src/linuxframestream.cpp src/exfatmodule.cpp
    src/gpl/CPP/7zip/Archive/CpioHandler.cpp
    src/CPP/7zip/Archive/GzHandler.cpp src/CPP/7zip/Archive/LzmaHandler.cpp
    src/initramfsdecoders.cpp src/initramfsreader.cpp src/initramfscache.cpp
    src/CPP/7zip/Common/LimitedStreams.cpp
    src/CPP/7zip/Compress/LzmaDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/BZip2Decoder.cpp
    third_party/7zip/CPP/7zip/Compress/BZip2Crc.cpp
    third_party/7zip/CPP/7zip/Compress/CopyCoder.cpp
    third_party/7zip/CPP/7zip/Compress/DeflateDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/BitlDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/LzOutWindow.cpp
    third_party/7zip/CPP/7zip/Archive/Common/OutStreamWithCRC.cpp
    third_party/7zip/CPP/7zip/Archive/Common/ItemNameUtils.cpp
    third_party/7zip/CPP/7zip/Common/CWrappers.cpp
    third_party/7zip/CPP/7zip/Common/InBuffer.cpp
    third_party/7zip/CPP/7zip/Common/OutBuffer.cpp
    third_party/7zip/CPP/Windows/TimeUtils.cpp
    third_party/7zip/CPP/Common/CRC.cpp
    third_party/7zip/CPP/Common/XzCrc64Init.cpp
    third_party/7zip/CPP/Common/MyVector.cpp
    third_party/7zip/CPP/Common/StringToInt.cpp
    third_party/7zip/C/7zCrc.c third_party/7zip/C/7zCrcOpt.c
    third_party/7zip/C/CpuArch.c third_party/7zip/C/Alloc.c third_party/7zip/C/LzmaDec.c
    third_party/7zip/C/Lzma2Dec.c third_party/7zip/C/Xz.c src/C/XzDec.c
    src/C/XzCrc64.c
    src/C/Sha256.c third_party/7zip/C/Sha256Opt.c
    third_party/7zip/C/Bra.c third_party/7zip/C/Bra86.c third_party/7zip/C/BraIA64.c
    third_party/7zip/C/Delta.c src/C/ZstdDec.c src/C/Xxh64.c
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
target_include_directories(sevenzip_linux_frames PRIVATE src src/C third_party/7zip/C third_party/7zip/CPP
    third_party/7zip/CPP/7zip/Archive)
target_compile_definitions(sevenzip_linux_frames PRIVATE Z7_ST)
set_source_files_properties(src/CPP/7zip/Archive/GzHandler.cpp
    src/CPP/7zip/Archive/LzmaHandler.cpp PROPERTIES
    COMPILE_DEFINITIONS SEVENZIP_INITRAMFS_FRAME_ONLY)
target_link_libraries(sevenzip_linux_frames PRIVATE sevenzip_linux_frames_core sevenzip_zstd
    sevenzip_liblzma_crc64)
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

# The GPL reader sources use their own upstream-shaped include root.
target_include_directories(sevenzip_linux_frames BEFORE PRIVATE src/gpl/CPP src/CPP third_party/7zip)
