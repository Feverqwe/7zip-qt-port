# Linux read-side sources retain C linkage and their original source language.
add_library(sevenzip_btrfs_core STATIC
    src/formats/btrfs/btrfsreader.c
    src/formats/btrfs/btrfsfilereader.c
    src/formats/btrfs/ctree.c
    src/formats/btrfs/compression.c
    src/formats/btrfs/zlib.c
    src/formats/btrfs/lzo.c
    src/formats/btrfs/zstd.c
    src/formats/btrfs/disk-io.c
    src/formats/btrfs/inode.c
    src/formats/btrfs/inode-item.c
    src/formats/btrfs/file-item.c
    src/formats/btrfs/fs.c
    src/formats/btrfs/root-tree.c
    src/formats/btrfs/xattr.c
    src/formats/btrfs/tree-checker.c
    src/formats/btrfs/volumes.c
    src/compat/linux/core/rbtree.c
    src/checksums/linux/crc32-main.c
    src/crypto/primitives/blake2b.c
    src/codecs/lzo/lzo1x_decompress_safe.c
    src/codecs/zstd/ZstdDec.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Alloc.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Sha256.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Sha256Opt.c
    src/checksums/xxhash/Xxh64.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/CpuArch.c)
target_include_directories(sevenzip_btrfs_core PRIVATE src src/compat/linux/include
    ${SEVENZIP_DEPENDENCY_DIR}/linux/include)
target_link_libraries(sevenzip_btrfs_core PUBLIC sevenzip_zstd sevenzip_zisofs_zlib
    sevenzip_liblzma_crc sevenzip_crc32c)
set_target_properties(sevenzip_btrfs_core PROPERTIES
    AUTOMOC OFF AUTOUIC OFF POSITION_INDEPENDENT_CODE ON
    C_STANDARD 11 C_STANDARD_REQUIRED ON)

add_library(sevenzip_btrfs MODULE
    src/formats/btrfs/btrfshandler.cpp src/formats/btrfs/btrfsarchivereader.cpp src/formats/btrfs/btrfsarchiveattributes.cpp
    src/formats/btrfs/btrfsarchiveproperties.cpp
    src/archive/resources/archiveattributes.cpp src/formats/common/moduleexports.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/ArchiveExports.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/StreamUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/StreamObjects.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/InBuffer.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/OutBuffer.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/PropId.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/PropVariant.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/TimeUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyWindows.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyString.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyVector.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/IntToString.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/StringConvert.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/UTFConvert.cpp)
target_include_directories(sevenzip_btrfs PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive)
target_link_libraries(sevenzip_btrfs PRIVATE sevenzip_btrfs_core Threads::Threads)
set_target_properties(sevenzip_btrfs PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX "")
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_btrfs PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_btrfs PRIVATE src/formats/common/moduleexports.def)
    target_link_libraries(sevenzip_btrfs PRIVATE oleaut32)
endif()

function(sevenzip_deploy_btrfs)
    add_custom_target(sevenzip_btrfs_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_btrfs>"
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats/$<TARGET_FILE_NAME:sevenzip_btrfs>"
        DEPENDS sevenzip_btrfs VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_btrfs_runtime)
endfunction()

if(BUILD_TESTING)
    qt_add_executable(btrfs_reader_tests tests/formats/btrfs/btrfs_reader_tests.cpp)
    set_target_properties(btrfs_reader_tests PROPERTIES AUTOMOC ON)
    target_include_directories(btrfs_reader_tests PRIVATE src tests)
    target_link_libraries(btrfs_reader_tests PRIVATE sevenzip_btrfs_core Qt6::Test)
    target_compile_definitions(btrfs_reader_tests PRIVATE
        "SEVENZIP_BTRFS_FIXTURES=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/btrfs\"")
    add_test(NAME btrfs_reader COMMAND btrfs_reader_tests -v1)
    set_tests_properties(btrfs_reader PROPERTIES TIMEOUT 90)
    qt_add_executable(btrfs_module_tests tests/formats/btrfs/btrfs_module_tests.cpp
        src/archive/backend/archivebackend.cpp src/archive/backend/archivebackend_validation.cpp src/archive/backend/archivebackend.h)
    set_target_properties(btrfs_module_tests PROPERTIES AUTOMOC ON)
    target_include_directories(btrfs_module_tests PRIVATE src tests ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(btrfs_module_tests PRIVATE sevenzip_panel sevenzip_btrfs_core Qt6::Test)
    target_compile_definitions(btrfs_module_tests PRIVATE
        "SEVENZIP_BTRFS_MODULE=\"$<TARGET_FILE:sevenzip_btrfs>\""
        "SEVENZIP_MBR_MODULE=\"$<TARGET_FILE:sevenzip_mbr>\""
        "SEVENZIP_BTRFS_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_BTRFS_FIXTURES=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/btrfs\"")
    if(WIN32)
        target_link_libraries(btrfs_module_tests PRIVATE oleaut32)
    endif()
    add_dependencies(btrfs_module_tests sevenzip_btrfs_runtime sevenzip_mbr_runtime)
    add_test(NAME btrfs_module COMMAND btrfs_module_tests -silent)
    set_tests_properties(btrfs_module PROPERTIES TIMEOUT 90)
    qt_add_executable(btrfs_multidevice_tests tests/formats/btrfs/btrfs_multidevice_tests.cpp)
    set_target_properties(btrfs_multidevice_tests PROPERTIES AUTOMOC ON)
    target_include_directories(btrfs_multidevice_tests PRIVATE src tests ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(btrfs_multidevice_tests PRIVATE sevenzip_panel sevenzip_btrfs_core Qt6::Test)
    target_compile_definitions(btrfs_multidevice_tests PRIVATE
        "SEVENZIP_BTRFS_MODULE=\"$<TARGET_FILE:sevenzip_btrfs>\""
        "SEVENZIP_BTRFS_FIXTURES=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/btrfs\"")
    if(WIN32)
        target_link_libraries(btrfs_multidevice_tests PRIVATE oleaut32)
    endif()
    add_dependencies(btrfs_multidevice_tests sevenzip_btrfs_runtime)
    add_test(NAME btrfs_multidevice COMMAND btrfs_multidevice_tests -silent)
    set_tests_properties(btrfs_multidevice PROPERTIES TIMEOUT 90)
endif()
