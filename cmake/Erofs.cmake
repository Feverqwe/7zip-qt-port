# Linux C read-side code is isolated from the core engine's codec licenses.
add_library(sevenzip_erofs_core STATIC
    src/erofsreader.c src/fs/erofs/data.c src/fs/erofs/dir.c
    src/fs/erofs/xattr.c src/fs/posix_acl.c src/lib/xxhash.c
    src/fs/erofs/inode.c src/fs/erofs/super.c src/fs/erofs/zmap.c
    src/fs/erofs/decompressor.c src/fs/erofs/decompressor_lzma.c
    src/fs/erofs/decompressor_deflate.c src/fs/erofs/decompressor_zstd.c
    src/lib/crc/crc32-main.c src/lib/lz4/lz4_decompress.c
    src/erofsnativedecoders.c third_party/linux/lib/xz/xz_dec_lzma2.c
    third_party/7zip/C/ZstdDec.c third_party/7zip/C/Xxh64.c
    third_party/7zip/C/Alloc.c third_party/7zip/C/CpuArch.c)
target_include_directories(sevenzip_erofs_core PRIVATE src src/lib/xz)
set_target_properties(sevenzip_erofs_core PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 11 C_STANDARD_REQUIRED ON)

add_library(sevenzip_erofs MODULE
    src/erofshandler.cpp src/erofsarchivereader.cpp src/erofsarchiveattributes.cpp src/exfatmodule.cpp
    third_party/7zip/CPP/7zip/Archive/ArchiveExports.cpp
    third_party/7zip/CPP/7zip/Common/StreamUtils.cpp
    third_party/7zip/CPP/7zip/Common/StreamObjects.cpp
    third_party/7zip/CPP/7zip/Common/InBuffer.cpp
    third_party/7zip/CPP/7zip/Common/OutBuffer.cpp
    third_party/7zip/CPP/7zip/Common/PropId.cpp
    third_party/7zip/CPP/7zip/Compress/DeflateDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/BitlDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/LzOutWindow.cpp
    third_party/7zip/CPP/Windows/PropVariant.cpp
    third_party/7zip/CPP/Windows/TimeUtils.cpp
    third_party/7zip/CPP/Common/MyWindows.cpp
    third_party/7zip/CPP/Common/MyString.cpp
    third_party/7zip/CPP/Common/MyVector.cpp
    third_party/7zip/CPP/Common/IntToString.cpp
    third_party/7zip/CPP/Common/StringConvert.cpp
    third_party/7zip/CPP/Common/UTFConvert.cpp)
target_include_directories(sevenzip_erofs PRIVATE src third_party/7zip/CPP
    third_party/7zip/CPP/7zip/Archive)
target_link_libraries(sevenzip_erofs PRIVATE sevenzip_erofs_core Threads::Threads)
set_target_properties(sevenzip_erofs PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX "")
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_erofs PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_erofs PRIVATE src/exfatmodule.def)
    target_link_libraries(sevenzip_erofs PRIVATE oleaut32)
endif()
function(sevenzip_deploy_erofs)
    add_custom_target(sevenzip_erofs_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_erofs>"
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats/$<TARGET_FILE_NAME:sevenzip_erofs>"
        DEPENDS sevenzip_erofs VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_erofs_runtime)
endfunction()
if(BUILD_TESTING)
    qt_add_executable(erofs_module_tests tests/erofs_module_tests.cpp)
    set_target_properties(erofs_module_tests PROPERTIES AUTOMOC ON)
    target_include_directories(erofs_module_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(erofs_module_tests PRIVATE sevenzip_panel Qt6::Test)
    target_compile_definitions(erofs_module_tests PRIVATE
        "SEVENZIP_EROFS_MODULE=\"$<TARGET_FILE:sevenzip_erofs>\""
        "SEVENZIP_EROFS_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_EROFS_FIXTURES=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/erofs\"")
    if(WIN32)
        target_link_libraries(erofs_module_tests PRIVATE oleaut32)
    endif()
    add_dependencies(erofs_module_tests sevenzip_erofs_runtime)
    add_test(NAME erofs_module COMMAND erofs_module_tests -silent)
    set_tests_properties(erofs_module PROPERTIES TIMEOUT 60)
endif()
