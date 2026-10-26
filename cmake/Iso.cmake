# Official zlib decoder API; unchanged C sources, shared by the GPL readers.
add_library(sevenzip_zisofs_zlib STATIC
    third_party/zlib/adler32.c third_party/zlib/inflate.c
    third_party/zlib/inffast.c third_party/zlib/inftrees.c third_party/zlib/zutil.c)
target_include_directories(sevenzip_zisofs_zlib PUBLIC third_party/zlib)
target_compile_definitions(sevenzip_zisofs_zlib PUBLIC Z_PREFIX)
target_compile_definitions(sevenzip_zisofs_zlib PRIVATE NO_GZIP)
set_target_properties(sevenzip_zisofs_zlib PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 99 C_STANDARD_REQUIRED ON
    C_VISIBILITY_PRESET hidden)
if(MSVC)
    target_compile_definitions(sevenzip_zisofs_zlib PRIVATE _CRT_SECURE_NO_DEPRECATE)
endif()

# Linux Rock Ridge stays in an isolated archive-only GPL module.
add_library(sevenzip_iso MODULE
    src/gpl/CPP/7zip/Archive/Iso/IsoIn.cpp src/gpl/CPP/7zip/Archive/Iso/IsoHandler.cpp
    src/gpl/CPP/7zip/Archive/Iso/IsoRegister.cpp src/gpl/CPP/7zip/Archive/Iso/IsoLinux.cpp
    src/gpl/CPP/7zip/Archive/Iso/IsoZisofs.cpp
    src/CPP/7zip/Common/MethodProps.cpp third_party/7zip/CPP/Common/StringToInt.cpp
    third_party/7zip/CPP/7zip/Archive/Iso/IsoHeader.cpp
    src/fs/isofs/rock.c src/fs/isofs/inode.c src/fs/isofs/util.c src/kernel/time/time.c src/exfatmodule.cpp
    src/fs/isofs/compress.c
    src/fs/isofs/dir.c src/fs/isofs/joliet.c src/fs/nls/nls_base.c
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
    src/include third_party/linux/include)
target_compile_definitions(sevenzip_iso PRIVATE CONFIG_ZISOFS Z7_ST)
target_link_libraries(sevenzip_iso PRIVATE Threads::Threads sevenzip_zisofs_zlib sevenzip_nls_system)
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
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_iso>"
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats/$<TARGET_FILE_NAME:sevenzip_iso>"
        DEPENDS sevenzip_iso VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_iso_runtime)
endfunction()

# The GPL reader sources use their own upstream-shaped include root.
target_include_directories(sevenzip_iso BEFORE PRIVATE src/gpl/CPP
    src/gpl/CPP/7zip/Archive/Iso src/CPP third_party/7zip)
