# Linux-derived C readers remain in a separate GPL format module.
add_library(sevenzip_ntfs MODULE
    src/CPP/7zip/Archive/NtfsHandler.cpp src/exfatmodule.cpp
    src/ntfsdecoder.c src/fs/ntfs3/lznt.c
    src/ntfswofstream.cpp src/fs/ntfs3/attrib.c src/fs/ntfs3/frecord.c
    src/fs/ntfs3/attrlist.c src/fs/ntfs3/fsntfs.c
    src/fs/ntfs3/xattr.c src/fs/ntfs3/super.c
    src/ntfssecurity.c src/fs/ntfs3/index.c
    src/fs/ntfs3/lib/decompress_common.c src/fs/ntfs3/lib/lzx_decompress.c
    src/fs/ntfs3/lib/xpress_decompress.c
    third_party/7zip/CPP/7zip/Archive/ArchiveExports.cpp
    third_party/7zip/CPP/7zip/Common/StreamUtils.cpp
    third_party/7zip/CPP/7zip/Common/StreamObjects.cpp
    third_party/7zip/CPP/7zip/Common/ProgressUtils.cpp
    third_party/7zip/CPP/7zip/Common/PropId.cpp
    src/CPP/7zip/Common/MethodProps.cpp
    third_party/7zip/CPP/7zip/Archive/Common/DummyOutStream.cpp
    third_party/7zip/CPP/7zip/Compress/CopyCoder.cpp
    third_party/7zip/CPP/Windows/PropVariant.cpp
    third_party/7zip/CPP/Windows/PropVariantUtils.cpp
    third_party/7zip/CPP/Windows/PropVariantConv.cpp
    third_party/7zip/CPP/Windows/TimeUtils.cpp
    third_party/7zip/CPP/Common/MyWindows.cpp
    third_party/7zip/CPP/Common/MyString.cpp
    third_party/7zip/CPP/Common/MyVector.cpp
    third_party/7zip/CPP/Common/IntToString.cpp
    third_party/7zip/CPP/Common/StringConvert.cpp
    third_party/7zip/CPP/Common/StringToInt.cpp
    third_party/7zip/CPP/Common/UTFConvert.cpp
    third_party/7zip/C/CpuArch.c third_party/7zip/C/Alloc.c)
target_include_directories(sevenzip_ntfs PRIVATE src third_party/7zip/CPP
    third_party/7zip/CPP/7zip/Archive)
target_compile_definitions(sevenzip_ntfs PRIVATE SEVENZIP_LINUX_NTFS)
target_link_libraries(sevenzip_ntfs PRIVATE Threads::Threads)
set_target_properties(sevenzip_ntfs PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_ntfs PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_ntfs PRIVATE src/exfatmodule.def)
    target_link_libraries(sevenzip_ntfs PRIVATE oleaut32)
endif()
function(sevenzip_deploy_ntfs)
    add_custom_target(sevenzip_ntfs_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_ntfs>"
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats/$<TARGET_FILE_NAME:sevenzip_ntfs>"
        DEPENDS sevenzip_ntfs VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_ntfs_runtime)
endfunction()
