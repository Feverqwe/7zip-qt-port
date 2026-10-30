# Linux UDF allocation readers stay in an isolated GPL format module.
add_library(sevenzip_udf MODULE
    src/formats/udf/UdfIn.cpp src/formats/udf/UdfHandler.cpp
    src/formats/udf/UdfLinux.cpp src/formats/udf/UdfDiscovery.cpp src/formats/common/moduleexports.cpp
    src/formats/udf/inode.c src/formats/udf/directory.c src/formats/udf/reader.c src/formats/udf/dir.c
    src/formats/udf/misc.c src/checksums/linux/crc-itu-t.c
    src/formats/udf/partition.c src/formats/udf/super.c src/formats/udf/vat.c src/formats/udf/metadata.c
    src/formats/udf/discovery.c src/formats/udf/lowlevel.c src/formats/udf/unicode.c src/formats/udf/symlink.c
    src/text/nls/nls_base.c
    src/formats/udf/udftime.c src/compat/linux/time/time.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/ArchiveExports.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Common/FindSignature.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Common/ItemNameUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/StreamUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/StreamObjects.cpp
    src/archive/streams/LimitedStreams.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/ProgressUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/PropId.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/CopyCoder.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/PropVariant.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/PropVariantUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/PropVariantConv.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/TimeUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyWindows.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyString.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyVector.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyMap.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/IntToString.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/StringConvert.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/UTFConvert.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/CpuArch.c ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Alloc.c)

target_include_directories(sevenzip_udf PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP
    ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP/7zip/Archive/Udf ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Udf)
set_source_files_properties(src/formats/udf/inode.c src/formats/udf/directory.c src/formats/udf/reader.c
    src/formats/udf/dir.c
    src/formats/udf/misc.c src/checksums/linux/crc-itu-t.c
    src/formats/udf/partition.c src/formats/udf/super.c src/formats/udf/vat.c src/formats/udf/metadata.c
    src/formats/udf/discovery.c src/formats/udf/lowlevel.c src/formats/udf/unicode.c src/formats/udf/symlink.c src/formats/udf/udftime.c
    PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/src/formats/udf/include")
target_compile_definitions(sevenzip_udf PRIVATE Z7_ST)
target_link_libraries(sevenzip_udf PRIVATE Threads::Threads)
set_target_properties(sevenzip_udf PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_udf PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_udf PRIVATE src/formats/common/moduleexports.def)
    target_link_libraries(sevenzip_udf PRIVATE oleaut32)
endif()
function(sevenzip_deploy_udf)
    add_custom_target(sevenzip_udf_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_udf>"
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats/$<TARGET_FILE_NAME:sevenzip_udf>"
        DEPENDS sevenzip_udf VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_udf_runtime)
endfunction()

# The GPL reader sources use their own upstream-shaped include root.
target_include_directories(sevenzip_udf BEFORE PRIVATE ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP
    ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP/7zip/Archive/Udf ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP ${SEVENZIP_7ZIP_SOURCE_DIR})
