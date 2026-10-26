# Linux read-side sources retain C linkage and their original source language.
add_library(sevenzip_btrfs_core STATIC
    src/btrfsreader.c
    src/btrfsfilereader.c
    src/fs/btrfs/ctree.c
    src/fs/btrfs/compression.c
    src/fs/btrfs/zlib.c
    src/fs/btrfs/lzo.c
    src/fs/btrfs/zstd.c
    src/fs/btrfs/disk-io.c
    src/fs/btrfs/inode.c
    src/fs/btrfs/inode-item.c
    src/fs/btrfs/file-item.c
    src/fs/btrfs/fs.c
    src/fs/btrfs/root-tree.c
    src/fs/btrfs/xattr.c
    src/fs/btrfs/tree-checker.c
    src/fs/btrfs/volumes.c
    src/lib/rbtree.c
    src/lib/crc/crc32-main.c
    src/lib/crypto/blake2b.c
    src/lib/lzo/lzo1x_decompress_safe.c
    src/C/ZstdDec.c
    third_party/7zip/C/Alloc.c
    third_party/7zip/C/Sha256.c
    third_party/7zip/C/Sha256Opt.c
    src/C/Xxh64.c
    third_party/7zip/C/CpuArch.c)
target_include_directories(sevenzip_btrfs_core PRIVATE src src/include
    third_party/linux/include)
target_link_libraries(sevenzip_btrfs_core PUBLIC sevenzip_zstd sevenzip_zisofs_zlib
    sevenzip_liblzma_crc sevenzip_crc32c)
set_target_properties(sevenzip_btrfs_core PROPERTIES
    AUTOMOC OFF AUTOUIC OFF POSITION_INDEPENDENT_CODE ON
    C_STANDARD 11 C_STANDARD_REQUIRED ON)

add_library(sevenzip_btrfs MODULE
    src/btrfshandler.cpp src/btrfsarchivereader.cpp src/btrfsarchiveattributes.cpp
    src/btrfsarchiveproperties.cpp
    src/archiveattributes.cpp src/exfatmodule.cpp
    third_party/7zip/CPP/7zip/Archive/ArchiveExports.cpp
    third_party/7zip/CPP/7zip/Common/StreamUtils.cpp
    third_party/7zip/CPP/7zip/Common/StreamObjects.cpp
    third_party/7zip/CPP/7zip/Common/InBuffer.cpp
    third_party/7zip/CPP/7zip/Common/OutBuffer.cpp
    third_party/7zip/CPP/7zip/Common/PropId.cpp
    third_party/7zip/CPP/Windows/PropVariant.cpp
    third_party/7zip/CPP/Windows/TimeUtils.cpp
    third_party/7zip/CPP/Common/MyWindows.cpp
    third_party/7zip/CPP/Common/MyString.cpp
    third_party/7zip/CPP/Common/MyVector.cpp
    third_party/7zip/CPP/Common/IntToString.cpp
    third_party/7zip/CPP/Common/StringConvert.cpp
    third_party/7zip/CPP/Common/UTFConvert.cpp)
target_include_directories(sevenzip_btrfs PRIVATE src third_party/7zip/CPP
    third_party/7zip/CPP/7zip/Archive)
target_link_libraries(sevenzip_btrfs PRIVATE sevenzip_btrfs_core Threads::Threads)
set_target_properties(sevenzip_btrfs PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX "")
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_btrfs PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_btrfs PRIVATE src/exfatmodule.def)
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
    qt_add_executable(btrfs_reader_tests tests/btrfs_reader_tests.cpp)
    set_target_properties(btrfs_reader_tests PROPERTIES AUTOMOC ON)
    target_include_directories(btrfs_reader_tests PRIVATE src tests)
    target_link_libraries(btrfs_reader_tests PRIVATE sevenzip_btrfs_core Qt6::Test)
    target_compile_definitions(btrfs_reader_tests PRIVATE
        "SEVENZIP_BTRFS_FIXTURES=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/btrfs\"")
    add_test(NAME btrfs_reader COMMAND btrfs_reader_tests -v1)
    set_tests_properties(btrfs_reader PROPERTIES TIMEOUT 90)
    qt_add_executable(btrfs_module_tests tests/btrfs_module_tests.cpp
        src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend.h)
    set_target_properties(btrfs_module_tests PROPERTIES AUTOMOC ON)
    target_include_directories(btrfs_module_tests PRIVATE src tests third_party/7zip/CPP)
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
    qt_add_executable(btrfs_multidevice_tests tests/btrfs_multidevice_tests.cpp)
    set_target_properties(btrfs_multidevice_tests PROPERTIES AUTOMOC ON)
    target_include_directories(btrfs_multidevice_tests PRIVATE src tests third_party/7zip/CPP)
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
