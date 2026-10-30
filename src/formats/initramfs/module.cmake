# Linux C framing/decoders stay in an isolated GPL module.
add_library(sevenzip_linux_frames_core STATIC
    src/codecs/frames/decompress_unlz4.c src/codecs/frames/decompress_unlzo.c
    src/codecs/lzo/lzo1x_decompress_safe.c
    src/checksums/linux/crc32-main.c)
target_include_directories(sevenzip_linux_frames_core PRIVATE src ${SEVENZIP_DEPENDENCY_DIR}/linux/include)
target_link_libraries(sevenzip_linux_frames_core PUBLIC sevenzip_lz4 sevenzip_zisofs_zlib
    sevenzip_liblzma_crc sevenzip_crc32c)
set_target_properties(sevenzip_linux_frames_core PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 11 C_STANDARD_REQUIRED ON)
add_library(sevenzip_linux_frames MODULE
    src/formats/initramfs/LinuxFrameHandler.cpp src/formats/initramfs/linuxframestream.cpp src/formats/common/moduleexports.cpp
    src/formats/cpio/CpioHandler.cpp
    src/formats/gzip/GzHandler.cpp src/formats/lzma/LzmaHandler.cpp
    src/formats/initramfs/initramfsdecoders.cpp src/formats/initramfs/initramfsreader.cpp src/formats/initramfs/initramfscache.cpp
    src/archive/streams/LimitedStreams.cpp
    src/codecs/lzma/LzmaDecoder.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/BZip2Decoder.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/BZip2Crc.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/CopyCoder.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/DeflateDecoder.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/BitlDecoder.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/LzOutWindow.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Common/OutStreamWithCRC.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Common/ItemNameUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/CWrappers.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/InBuffer.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/OutBuffer.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/TimeUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/CRC.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/XzCrc64Init.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyVector.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/StringToInt.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/7zCrc.c ${SEVENZIP_7ZIP_SOURCE_DIR}/C/7zCrcOpt.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/CpuArch.c ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Alloc.c ${SEVENZIP_7ZIP_SOURCE_DIR}/C/LzmaDec.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Lzma2Dec.c ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Xz.c src/codecs/xz/XzDec.c
    src/checksums/crc64/XzCrc64.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Sha256.c ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Sha256Opt.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Bra.c ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Bra86.c ${SEVENZIP_7ZIP_SOURCE_DIR}/C/BraIA64.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Delta.c src/codecs/zstd/ZstdDec.c src/checksums/xxhash/Xxh64.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/ArchiveExports.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Common/DummyOutStream.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Common/FindSignature.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/ProgressUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/StreamUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/PropId.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/PropVariant.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyWindows.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyString.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/IntToString.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/StringConvert.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/UTFConvert.cpp)
target_include_directories(sevenzip_linux_frames PRIVATE src ${SEVENZIP_NATIVE_INCLUDE_DIR}/C ${SEVENZIP_7ZIP_SOURCE_DIR}/C ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive)
target_compile_definitions(sevenzip_linux_frames PRIVATE Z7_ST)
set_source_files_properties(src/formats/gzip/GzHandler.cpp
    src/formats/lzma/LzmaHandler.cpp PROPERTIES
    COMPILE_DEFINITIONS SEVENZIP_INITRAMFS_FRAME_ONLY)
target_link_libraries(sevenzip_linux_frames PRIVATE sevenzip_linux_frames_core sevenzip_zstd
    sevenzip_liblzma_crc)
set_target_properties(sevenzip_linux_frames PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX "")
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_linux_frames PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_linux_frames PRIVATE src/formats/common/moduleexports.def)
    target_link_libraries(sevenzip_linux_frames PRIVATE oleaut32)
endif()
function(sevenzip_deploy_linux_frames)
    add_custom_target(sevenzip_linux_frames_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_linux_frames>"
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats/$<TARGET_FILE_NAME:sevenzip_linux_frames>"
        DEPENDS sevenzip_linux_frames VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_linux_frames_runtime)
endfunction()

# The GPL reader sources use their own upstream-shaped include root.
target_include_directories(sevenzip_linux_frames BEFORE PRIVATE ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP ${SEVENZIP_7ZIP_SOURCE_DIR})
