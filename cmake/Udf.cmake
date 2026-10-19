# Linux UDF allocation readers stay in an isolated GPL format module.
add_library(sevenzip_udf MODULE
    src/CPP/7zip/Archive/Udf/UdfIn.cpp src/CPP/7zip/Archive/Udf/UdfHandler.cpp
    src/CPP/7zip/Archive/Udf/UdfLinux.cpp src/exfatmodule.cpp
    src/fs/udf/inode.c src/fs/udf/directory.c src/fs/udf/reader.c
    third_party/7zip/CPP/7zip/Archive/ArchiveExports.cpp
    third_party/7zip/CPP/7zip/Archive/Common/FindSignature.cpp
    third_party/7zip/CPP/7zip/Archive/Common/ItemNameUtils.cpp
    third_party/7zip/CPP/7zip/Common/StreamUtils.cpp
    third_party/7zip/CPP/7zip/Common/StreamObjects.cpp
    src/CPP/7zip/Common/LimitedStreams.cpp
    third_party/7zip/CPP/7zip/Common/ProgressUtils.cpp
    third_party/7zip/CPP/7zip/Common/PropId.cpp
    third_party/7zip/CPP/7zip/Compress/CopyCoder.cpp
    third_party/7zip/CPP/Windows/PropVariant.cpp
    third_party/7zip/CPP/Windows/PropVariantUtils.cpp
    third_party/7zip/CPP/Windows/PropVariantConv.cpp
    third_party/7zip/CPP/Windows/TimeUtils.cpp
    third_party/7zip/CPP/Common/MyWindows.cpp
    third_party/7zip/CPP/Common/MyString.cpp
    third_party/7zip/CPP/Common/MyVector.cpp
    third_party/7zip/CPP/Common/MyMap.cpp
    third_party/7zip/CPP/Common/IntToString.cpp
    third_party/7zip/CPP/Common/StringConvert.cpp
    third_party/7zip/CPP/Common/UTFConvert.cpp
    third_party/7zip/C/CpuArch.c third_party/7zip/C/Alloc.c)

target_include_directories(sevenzip_udf PRIVATE src third_party/7zip/CPP
    src/CPP/7zip/Archive/Udf third_party/7zip/CPP/7zip/Archive/Udf)
set_source_files_properties(src/fs/udf/inode.c src/fs/udf/directory.c src/fs/udf/reader.c
    PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/src/fs/udf/include")
target_compile_definitions(sevenzip_udf PRIVATE SEVENZIP_LINUX_UDF Z7_ST)
target_link_libraries(sevenzip_udf PRIVATE Threads::Threads)
set_target_properties(sevenzip_udf PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_udf PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_udf PRIVATE src/exfatmodule.def)
    target_link_libraries(sevenzip_udf PRIVATE oleaut32)
endif()
function(sevenzip_deploy_udf)
    add_custom_target(sevenzip_udf_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_udf>"
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats/$<TARGET_FILE_NAME:sevenzip_udf>"
        DEPENDS sevenzip_udf VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_udf_runtime)
endfunction()
