# Official zlib decoder API; unchanged C sources, shared by the GPL readers.
add_library(sevenzip_zisofs_zlib STATIC
    ${SEVENZIP_DEPENDENCY_DIR}/zlib/adler32.c ${SEVENZIP_DEPENDENCY_DIR}/zlib/inflate.c
    ${SEVENZIP_DEPENDENCY_DIR}/zlib/inffast.c ${SEVENZIP_DEPENDENCY_DIR}/zlib/inftrees.c ${SEVENZIP_DEPENDENCY_DIR}/zlib/zutil.c)
target_include_directories(sevenzip_zisofs_zlib PUBLIC ${SEVENZIP_DEPENDENCY_DIR}/zlib)
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
    src/formats/iso/IsoIn.cpp src/formats/iso/IsoHandler.cpp
    src/formats/iso/IsoRegister.cpp src/formats/iso/IsoLinux.cpp
    src/formats/iso/IsoZisofs.cpp
    src/archive/compression/MethodProps.cpp ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/StringToInt.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Iso/IsoHeader.cpp
    src/formats/iso/rock.c src/formats/iso/inode.c src/formats/iso/util.c src/compat/linux/time/time.c src/formats/common/moduleexports.cpp
    src/formats/iso/compress.c
    src/formats/iso/dir.c src/formats/iso/joliet.c src/text/nls/nls_base.c
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
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/IntToString.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/StringConvert.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/UTFConvert.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/CpuArch.c ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Alloc.c)
target_include_directories(sevenzip_iso PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP
    ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP/7zip/Archive/Iso ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Iso ${SEVENZIP_DEPENDENCY_DIR}/linux/fs/isofs
    src/compat/linux/include ${SEVENZIP_DEPENDENCY_DIR}/linux/include)
target_compile_definitions(sevenzip_iso PRIVATE CONFIG_ZISOFS Z7_ST)
target_link_libraries(sevenzip_iso PRIVATE Threads::Threads sevenzip_zisofs_zlib sevenzip_nls_system)
set_target_properties(sevenzip_iso PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_iso PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_iso PRIVATE src/formats/common/moduleexports.def)
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
target_include_directories(sevenzip_iso BEFORE PRIVATE ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP
    ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP/7zip/Archive/Iso ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP ${SEVENZIP_7ZIP_SOURCE_DIR})
