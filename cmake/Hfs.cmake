# Linux HFS+ readers stay in a separate GPL format module.
add_library(sevenzip_hfs MODULE
    src/CPP/7zip/Archive/HfsDecmpfs.cpp
    src/gpl/CPP/7zip/Archive/HfsHandler.cpp src/exfatmodule.cpp
    src/hfsreader.c src/fs/namei.c src/fs/hfsplus/unicode.c src/fs/hfsplus/tables.c src/fs/hfsplus/dir.c
    src/fs/hfsplus/inode.c src/fs/hfsplus/bfind.c src/fs/hfsplus/xattr.c
    src/fs/hfsplus/wrapper.c src/fs/hfsplus/super.c
    third_party/7zip/CPP/7zip/Archive/ArchiveExports.cpp
    third_party/7zip/CPP/7zip/Common/StreamUtils.cpp
    third_party/7zip/CPP/7zip/Common/StreamObjects.cpp
    src/CPP/7zip/Common/LimitedStreams.cpp
    third_party/7zip/CPP/7zip/Common/InBuffer.cpp
    third_party/7zip/CPP/7zip/Common/OutBuffer.cpp
    third_party/7zip/CPP/7zip/Common/PropId.cpp
    src/CPP/7zip/Compress/ZlibDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/DeflateDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/BitlDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/LzOutWindow.cpp
    third_party/7zip/CPP/7zip/Compress/LzfseDecoder.cpp
    third_party/7zip/CPP/Windows/PropVariant.cpp
    third_party/7zip/CPP/Windows/PropVariantUtils.cpp
    third_party/7zip/CPP/Windows/PropVariantConv.cpp
    third_party/7zip/CPP/Windows/TimeUtils.cpp
    third_party/7zip/CPP/Common/MyWindows.cpp
    third_party/7zip/CPP/Common/MyString.cpp
    third_party/7zip/CPP/Common/MyVector.cpp
    third_party/7zip/CPP/Common/IntToString.cpp
    third_party/7zip/CPP/Common/StringConvert.cpp
    third_party/7zip/CPP/Common/UTFConvert.cpp
    third_party/7zip/C/CpuArch.c third_party/7zip/C/Alloc.c)
target_include_directories(sevenzip_hfs PRIVATE src third_party/7zip/CPP
    third_party/7zip/CPP/7zip/Archive)
target_compile_definitions(sevenzip_hfs PRIVATE Z7_ST)
target_link_libraries(sevenzip_hfs PRIVATE Threads::Threads sevenzip_zisofs_zlib)
target_include_directories(sevenzip_hfs PRIVATE third_party)
set_source_files_properties(src/CPP/7zip/Compress/ZlibDecoder.cpp PROPERTIES
    INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/Compress")
set_target_properties(sevenzip_hfs PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_hfs PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_hfs PRIVATE src/exfatmodule.def)
    target_link_libraries(sevenzip_hfs PRIVATE oleaut32)
endif()
function(sevenzip_deploy_hfs)
    add_custom_target(sevenzip_hfs_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_hfs>"
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats/$<TARGET_FILE_NAME:sevenzip_hfs>"
        DEPENDS sevenzip_hfs VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_hfs_runtime)
endfunction()

# The GPL reader sources use their own upstream-shaped include root.
target_include_directories(sevenzip_hfs BEFORE PRIVATE src/gpl/CPP src/CPP third_party/7zip)
