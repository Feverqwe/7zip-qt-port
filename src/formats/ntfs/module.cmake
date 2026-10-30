# Linux-derived C readers remain in a separate GPL format module.
# The same upstream dir.c owns its link conversion and directory read bodies;
# their existing portable inode contexts are compiled separately.
add_library(sevenzip_ntfs_directory OBJECT src/formats/ntfs/dir.c)
target_compile_definitions(sevenzip_ntfs_directory PRIVATE SEVENZIP_NTFS_DIRECTORY)
set_target_properties(sevenzip_ntfs_directory PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 11 C_STANDARD_REQUIRED ON)
add_library(sevenzip_ntfs MODULE
    src/formats/ntfs/NtfsHandler.cpp src/formats/common/moduleexports.cpp
    src/formats/ntfs/ntfsdecoder.c src/formats/ntfs/lznt.c src/formats/ntfs/bitmap.c src/compat/linux/core/hweight.c src/compat/linux/core/find_bit.c
    src/formats/ntfs/ntfswofstream.cpp src/formats/ntfs/attrib.c src/formats/ntfs/frecord.c
    src/formats/ntfs/attrlist.c src/formats/ntfs/fsntfs.c src/formats/ntfs/upcase.c
    src/formats/ntfs/xattr.c src/formats/ntfs/super.c
    src/formats/ntfs/ntfssecurity.c src/formats/ntfs/index.c src/formats/ntfs/ntfsdirectory.c
    $<TARGET_OBJECTS:sevenzip_ntfs_directory>
    src/formats/ntfs/ntfslink.c src/formats/ntfs/inode.c src/formats/ntfs/dir.c src/text/nls/nls_base.c
    src/formats/ntfs/lib/decompress_common.c src/formats/ntfs/lib/lzx_decompress.c
    src/formats/ntfs/lib/xpress_decompress.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/ArchiveExports.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/StreamUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/StreamObjects.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/ProgressUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/PropId.cpp
    src/archive/compression/MethodProps.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Common/DummyOutStream.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/CopyCoder.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/PropVariant.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/PropVariantUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/PropVariantConv.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/TimeUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyWindows.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyString.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyVector.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/IntToString.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/StringConvert.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/StringToInt.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/UTFConvert.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/CpuArch.c ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Alloc.c)
target_include_directories(sevenzip_ntfs PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive)
target_link_libraries(sevenzip_ntfs PRIVATE Threads::Threads)
set_target_properties(sevenzip_ntfs PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_ntfs PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_ntfs PRIVATE src/formats/common/moduleexports.def)
    target_link_libraries(sevenzip_ntfs PRIVATE oleaut32)
endif()
function(sevenzip_deploy_ntfs)
    add_custom_target(sevenzip_ntfs_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_ntfs>"
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats/$<TARGET_FILE_NAME:sevenzip_ntfs>"
        DEPENDS sevenzip_ntfs VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_ntfs_runtime)
endfunction()

# The GPL reader sources use their own upstream-shaped include root.
target_include_directories(sevenzip_ntfs BEFORE PRIVATE ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP ${SEVENZIP_7ZIP_SOURCE_DIR})
