# GPL metadata core is shared by the format module and its pure C API consumer.
add_library(sevenzip_squashfs_metadata STATIC
    src/squashfsmetadata.c src/squashfsio.c src/fs/squashfs/cache.c
    src/fs/squashfs/id.c src/fs/squashfs/fragment.c src/fs/squashfs/export.c
    src/fs/squashfs/xattr_id.c src/fs/squashfs/xattr.c
    src/fs/squashfs/block.c src/fs/squashfs/page_actor.c)
target_include_directories(sevenzip_squashfs_metadata PRIVATE src)
set_target_properties(sevenzip_squashfs_metadata PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 11 C_STANDARD_REQUIRED ON)

# Linux-derived C readers remain in a separate GPL format module.
add_library(sevenzip_squashfs MODULE
    src/CPP/7zip/Archive/SquashfsHandler.cpp src/exfatmodule.cpp src/archiveattributes.cpp
    src/squashfsdecoder.c src/fs/squashfs/decompressor.c
    src/fs/squashfs/decompressor_single.c
    src/fs/squashfs/lz4_wrapper.c src/fs/squashfs/lzo_wrapper.c src/fs/squashfs/xz_wrapper.c
    src/lib/lz4/lz4_decompress.c src/lib/lzo/lzo1x_decompress_safe.c
    third_party/7zip/CPP/7zip/Archive/ArchiveExports.cpp
    third_party/7zip/CPP/7zip/Common/StreamUtils.cpp
    third_party/7zip/CPP/7zip/Common/CWrappers.cpp
    third_party/7zip/CPP/7zip/Common/StreamObjects.cpp
    src/CPP/7zip/Common/LimitedStreams.cpp
    third_party/7zip/CPP/7zip/Common/InBuffer.cpp
    third_party/7zip/CPP/7zip/Common/OutBuffer.cpp
    third_party/7zip/CPP/7zip/Common/ProgressUtils.cpp
    third_party/7zip/CPP/7zip/Common/PropId.cpp
    third_party/7zip/CPP/7zip/Compress/CopyCoder.cpp
    third_party/7zip/CPP/7zip/Compress/ZlibDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/DeflateDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/BitlDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/LzOutWindow.cpp
    third_party/7zip/CPP/Windows/PropVariant.cpp
    third_party/7zip/CPP/Windows/PropVariantUtils.cpp
    third_party/7zip/CPP/Windows/PropVariantConv.cpp
    third_party/7zip/CPP/Windows/TimeUtils.cpp
    third_party/7zip/CPP/Common/CRC.cpp
    third_party/7zip/CPP/Common/MyWindows.cpp
    third_party/7zip/CPP/Common/MyString.cpp
    third_party/7zip/CPP/Common/MyVector.cpp
    third_party/7zip/CPP/Common/IntToString.cpp
    third_party/7zip/CPP/Common/StringConvert.cpp
    third_party/7zip/CPP/Common/UTFConvert.cpp
    third_party/7zip/C/7zCrc.c third_party/7zip/C/7zCrcOpt.c
    third_party/7zip/C/CpuArch.c third_party/7zip/C/Alloc.c third_party/7zip/C/LzmaDec.c
    third_party/7zip/C/Lzma2Dec.c third_party/7zip/C/Xz.c third_party/7zip/C/XzDec.c
    third_party/7zip/C/XzCrc64.c third_party/7zip/C/XzCrc64Opt.c
    third_party/7zip/CPP/Common/XzCrc64Init.cpp
    third_party/7zip/C/Sha256.c third_party/7zip/C/Sha256Opt.c
    third_party/7zip/C/Bra.c third_party/7zip/C/Bra86.c third_party/7zip/C/BraIA64.c
    third_party/7zip/C/Delta.c third_party/7zip/C/ZstdDec.c third_party/7zip/C/Xxh64.c)
target_include_directories(sevenzip_squashfs PRIVATE src third_party/7zip/CPP
    third_party/7zip/CPP/7zip/Archive)
target_compile_definitions(sevenzip_squashfs PRIVATE SEVENZIP_LINUX_SQUASHFS Z7_ST)
target_link_libraries(sevenzip_squashfs PRIVATE sevenzip_squashfs_metadata Threads::Threads)
set_target_properties(sevenzip_squashfs PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_squashfs PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_squashfs PRIVATE src/exfatmodule.def)
    target_link_libraries(sevenzip_squashfs PRIVATE oleaut32)
endif()
function(sevenzip_deploy_squashfs)
    add_custom_target(sevenzip_squashfs_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_squashfs>"
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats/$<TARGET_FILE_NAME:sevenzip_squashfs>"
        DEPENDS sevenzip_squashfs VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_squashfs_runtime)
endfunction()
