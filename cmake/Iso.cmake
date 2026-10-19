# Linux Rock Ridge stays in an isolated archive-only GPL module.
add_library(sevenzip_iso MODULE
    src/CPP/7zip/Archive/Iso/IsoIn.cpp src/CPP/7zip/Archive/Iso/IsoHandler.cpp
    src/CPP/7zip/Archive/Iso/IsoRegister.cpp src/CPP/7zip/Archive/Iso/IsoLinux.cpp
    src/CPP/7zip/Archive/Iso/IsoZisofs.cpp
    third_party/7zip/CPP/7zip/Archive/Iso/IsoHeader.cpp
    src/fs/isofs/rock.c src/fs/isofs/inode.c src/fs/isofs/util.c src/kernel/time/time.c src/exfatmodule.cpp
    src/fs/isofs/compress.c src/lib/zlib_inflate/inffast.c
    third_party/linux/lib/zlib_inflate/inflate.c third_party/linux/lib/zlib_inflate/inftrees.c
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
    third_party/7zip/CPP/Common/IntToString.cpp
    third_party/7zip/CPP/Common/StringConvert.cpp
    third_party/7zip/CPP/Common/UTFConvert.cpp
    third_party/7zip/C/CpuArch.c third_party/7zip/C/Alloc.c)
target_include_directories(sevenzip_iso PRIVATE src third_party/7zip/CPP
    src/CPP/7zip/Archive/Iso third_party/7zip/CPP/7zip/Archive/Iso third_party/linux/fs/isofs
    src/include third_party/linux/include third_party/linux/lib/zlib_inflate)
target_compile_definitions(sevenzip_iso PRIVATE SEVENZIP_LINUX_ISO CONFIG_ZISOFS Z7_ST)
target_link_libraries(sevenzip_iso PRIVATE Threads::Threads)
set_target_properties(sevenzip_iso PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_iso PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_iso PRIVATE src/exfatmodule.def)
    target_link_libraries(sevenzip_iso PRIVATE oleaut32)
endif()
function(sevenzip_deploy_iso)
    add_custom_target(sevenzip_iso_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_iso>"
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats/$<TARGET_FILE_NAME:sevenzip_iso>"
        DEPENDS sevenzip_iso VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_iso_runtime)
endfunction()
