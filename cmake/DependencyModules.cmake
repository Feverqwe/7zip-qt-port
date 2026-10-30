# Named modules wrap C++ integration APIs. Vendored implementations remain C.
include("${CMAKE_CURRENT_LIST_DIR}/ModuleToolchain.cmake")

function(sevenzip_add_dependency_facade dependency core)
    set(facade sevenzip_${dependency}_facade)
    if(SEVENZIP_ENABLE_DEPENDENCY_MODULES)
        string(TOUPPER "${dependency}" dependency_macro)
        add_library(${facade} STATIC)
        target_sources(${facade} PUBLIC FILE_SET CXX_MODULES
            FILES src/dependencies/${dependency}/${dependency}.cppm)
        target_link_libraries(${facade} PRIVATE ${core})
        target_compile_definitions(${facade} INTERFACE SEVENZIP_USE_${dependency_macro}_MODULE)
        set_target_properties(${facade} PROPERTIES
            AUTOMOC OFF AUTOUIC OFF POSITION_INDEPENDENT_CODE ON)
        sevenzip_configure_module_cache(${facade})
    else()
        add_library(${facade} INTERFACE)
        target_link_libraries(${facade} INTERFACE ${core})
    endif()
    target_include_directories(${facade} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")
endfunction()

sevenzip_add_dependency_facade(zlib sevenzip_zisofs_zlib)
sevenzip_add_dependency_facade(brotli sevenzip_brotli_core)
sevenzip_add_dependency_facade(lz4 sevenzip_lz4_frame_core)
sevenzip_add_dependency_facade(lzlib sevenzip_lzlib_core)
sevenzip_add_dependency_facade(smb2 smb2)
sevenzip_add_dependency_facade(grub_xfs sevenzip_xfs_core)
sevenzip_add_dependency_facade(linux_erofs sevenzip_erofs_core)
if(SEVENZIP_ENABLE_DEPENDENCY_MODULES)
    # Qt disables automatic scanning on qt_add_executable targets. This shared
    # source imports the facade in both the plugin and its Qt test executable.
    set_source_files_properties(src/formats/squashfs/squashfszlib.cpp
        src/codecs/brotli/BrotliDecoder.cpp src/formats/brotli/BrotliHandler.cpp
        src/formats/lz4/lz4framestream.cpp src/formats/lzip/lzipstream.cpp src/mount/smb/smbreadonlyserver.cpp src/formats/xfs/xfsarchivereader.cpp
        src/formats/erofs/erofsarchivereader.cpp src/formats/erofs/erofsarchiveattributes.cpp src/formats/erofs/erofshandler.cpp
        tests/formats/lz4/lz4_frame_tests.cpp tests/formats/lzip/lzip_tests.cpp tests/mount/smb_readonly_server_tests.cpp
        PROPERTIES CXX_SCAN_FOR_MODULES ON)
    sevenzip_configure_module_cache(sevenzip_squashfs)
    sevenzip_configure_module_cache(sevenzip_brotli)
    sevenzip_configure_module_cache(sevenzip_lz4_frame)
    sevenzip_configure_module_cache(sevenzip_lzip)
    sevenzip_configure_module_cache(sevenzip_smb)
    sevenzip_configure_module_cache(sevenzip_xfs)
    sevenzip_configure_module_cache(sevenzip_erofs)
    if(BUILD_TESTING)
        sevenzip_configure_module_cache(filesystem_zlib_tests)
        sevenzip_configure_module_cache(lz4_frame_tests)
        sevenzip_configure_module_cache(lzip_tests)
    endif()
endif()
target_link_libraries(sevenzip_squashfs PRIVATE sevenzip_zlib_facade)
target_link_libraries(sevenzip_brotli PRIVATE sevenzip_brotli_facade)
target_link_libraries(sevenzip_lz4_frame PRIVATE sevenzip_lz4_facade)
target_link_libraries(sevenzip_lzip PRIVATE sevenzip_lzlib_facade)
target_link_libraries(sevenzip_smb PRIVATE sevenzip_smb2_facade)
target_link_libraries(sevenzip_xfs PRIVATE sevenzip_grub_xfs_facade)
target_link_libraries(sevenzip_erofs PRIVATE sevenzip_linux_erofs_facade)
if(BUILD_TESTING)
    target_link_libraries(filesystem_zlib_tests PRIVATE sevenzip_zlib_facade)
    target_link_libraries(lz4_frame_tests PRIVATE sevenzip_lz4_facade)
    target_link_libraries(lzip_tests PRIVATE sevenzip_lzlib_facade)
endif()
