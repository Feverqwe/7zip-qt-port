# Linux C read-side code is isolated from the core engine's codec licenses.
add_library(sevenzip_erofs_liblzma STATIC
    ${SEVENZIP_DEPENDENCY_DIR}/xz/src/liblzma/common/common.c
    ${SEVENZIP_DEPENDENCY_DIR}/xz/src/liblzma/common/microlzma_decoder.c
    ${SEVENZIP_DEPENDENCY_DIR}/xz/src/liblzma/lz/lz_decoder.c
    ${SEVENZIP_DEPENDENCY_DIR}/xz/src/liblzma/lzma/lzma_decoder.c)
target_include_directories(sevenzip_erofs_liblzma PUBLIC ${SEVENZIP_DEPENDENCY_DIR}/xz/src/liblzma/api
    PRIVATE ${SEVENZIP_DEPENDENCY_DIR}/xz/src/common ${SEVENZIP_DEPENDENCY_DIR}/xz/src/liblzma/common
    ${SEVENZIP_DEPENDENCY_DIR}/xz/src/liblzma/lz ${SEVENZIP_DEPENDENCY_DIR}/xz/src/liblzma/lzma
    ${SEVENZIP_DEPENDENCY_DIR}/xz/src/liblzma/rangecoder)
target_compile_definitions(sevenzip_erofs_liblzma PUBLIC LZMA_API_STATIC
    PRIVATE HAVE_STDBOOL_H HAVE__BOOL HAVE_STDINT_H HAVE_INTTYPES_H HAVE_DECODER_LZMA1)
set_target_properties(sevenzip_erofs_liblzma PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 11 C_STANDARD_REQUIRED ON
    C_VISIBILITY_PRESET hidden)

add_library(sevenzip_erofs_core STATIC
    src/formats/erofs/erofsreader.c src/formats/erofs/data.c src/formats/erofs/dir.c
    src/formats/erofs/xattr.c src/compat/linux/filesystem/posix_acl.c src/compat/linux/filesystem/fs_dirent.c src/compat/linux/core/xxhash.c
    src/compat/linux/core/string.c src/compat/linux/util.c
    src/formats/erofs/inode.c src/formats/erofs/super.c src/formats/erofs/zmap.c
    src/formats/erofs/decompressor.c src/formats/erofs/decompressor_lzma.c
    src/formats/erofs/decompressor_deflate.c src/formats/erofs/decompressor_zstd.c
    src/checksums/linux/crc32-main.c
    src/formats/erofs/erofsnativedecoders.c
    src/codecs/zstd/ZstdDec.c src/checksums/xxhash/Xxh64.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Alloc.c ${SEVENZIP_7ZIP_SOURCE_DIR}/C/CpuArch.c)
target_include_directories(sevenzip_erofs_core PRIVATE src)
target_link_libraries(sevenzip_erofs_core PUBLIC sevenzip_lz4 sevenzip_zstd
    sevenzip_erofs_liblzma sevenzip_zisofs_zlib sevenzip_liblzma_crc sevenzip_crc32c)
set_target_properties(sevenzip_erofs_core PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 11 C_STANDARD_REQUIRED ON)

add_library(sevenzip_erofs MODULE
    src/formats/erofs/erofshandler.cpp src/formats/erofs/erofsarchivereader.cpp src/formats/erofs/erofsarchiveattributes.cpp
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
target_include_directories(sevenzip_erofs PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive)
target_link_libraries(sevenzip_erofs PRIVATE Threads::Threads)
set_target_properties(sevenzip_erofs PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX "")
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_erofs PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_erofs PRIVATE src/formats/common/moduleexports.def)
    target_link_libraries(sevenzip_erofs PRIVATE oleaut32)
endif()
function(sevenzip_deploy_erofs)
    add_custom_target(sevenzip_erofs_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_erofs>"
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats/$<TARGET_FILE_NAME:sevenzip_erofs>"
        DEPENDS sevenzip_erofs VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_erofs_runtime)
endfunction()
if(BUILD_TESTING)
    qt_add_executable(filesystem_zlib_tests tests/ui/filesystem_zlib_tests.cpp src/formats/squashfs/squashfszlib.cpp
        ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/StreamUtils.cpp
        ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/OutBuffer.cpp)
    set_target_properties(filesystem_zlib_tests PROPERTIES AUTOMOC ON)
    target_include_directories(filesystem_zlib_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP ${SEVENZIP_7ZIP_SOURCE_DIR})
    target_link_libraries(filesystem_zlib_tests PRIVATE sevenzip_erofs_core Qt6::Test)
    if(WIN32)
        target_link_libraries(filesystem_zlib_tests PRIVATE oleaut32)
    endif()
    add_test(NAME filesystem_zlib COMMAND filesystem_zlib_tests -silent)
    set_tests_properties(filesystem_zlib PROPERTIES TIMEOUT 30)
    qt_add_executable(erofs_reader_tests tests/formats/erofs/erofs_reader_tests.cpp)
    set_target_properties(erofs_reader_tests PROPERTIES AUTOMOC ON)
    target_include_directories(erofs_reader_tests PRIVATE src)
    target_link_libraries(erofs_reader_tests PRIVATE sevenzip_erofs_core Qt6::Test)
    target_compile_definitions(erofs_reader_tests PRIVATE
        "SEVENZIP_EROFS_FIXTURES=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/erofs\"")
    add_test(NAME erofs_reader COMMAND erofs_reader_tests -silent)
    set_tests_properties(erofs_reader PROPERTIES TIMEOUT 30)
    qt_add_executable(erofs_module_tests tests/formats/erofs/erofs_module_tests.cpp)
    set_target_properties(erofs_module_tests PROPERTIES AUTOMOC ON)
    target_include_directories(erofs_module_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(erofs_module_tests PRIVATE sevenzip_panel Qt6::Test)
    target_compile_definitions(erofs_module_tests PRIVATE
        "SEVENZIP_EROFS_MODULE=\"$<TARGET_FILE:sevenzip_erofs>\""
        "SEVENZIP_EROFS_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_EROFS_FIXTURES=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/erofs\"")
    if(WIN32)
        target_link_libraries(erofs_module_tests PRIVATE oleaut32)
    endif()
    add_dependencies(erofs_module_tests sevenzip_erofs_runtime)
    add_test(NAME erofs_module COMMAND erofs_module_tests -silent)
    set_tests_properties(erofs_module PROPERTIES TIMEOUT 60)
endif()
