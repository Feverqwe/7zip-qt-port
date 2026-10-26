# Named modules wrap C++ integration APIs. Vendored implementations remain C.
function(sevenzip_configure_module_cache target)
    if(NOT MSVC)
        return()
    endif()
    get_target_property(sevenzip_launcher ${target} CXX_COMPILER_LAUNCHER)
    if(NOT sevenzip_launcher)
        return()
    endif()
    list(GET sevenzip_launcher 0 sevenzip_cache_executable)
    get_filename_component(sevenzip_cache_name "${sevenzip_cache_executable}" NAME_WE)
    if(sevenzip_cache_name STREQUAL "ccache")
        find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
        set_property(TARGET ${target} PROPERTY CXX_COMPILER_LAUNCHER
            "${Python3_EXECUTABLE};${CMAKE_CURRENT_SOURCE_DIR}/scripts/lib/module_cache_launcher.py;${sevenzip_launcher}")
    endif()
endfunction()

set(sevenzip_modules_supported OFF)
if(MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.34)
    set(sevenzip_modules_supported ON)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16)
    set(sevenzip_modules_supported ON)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
        AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16)
    get_filename_component(sevenzip_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    string(REGEX MATCH "^[0-9]+" sevenzip_clang_major "${CMAKE_CXX_COMPILER_VERSION}")
    find_program(sevenzip_clang_scan_deps NAMES "clang-scan-deps-${sevenzip_clang_major}" clang-scan-deps
        HINTS "${sevenzip_compiler_dir}")
    if(CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS OR sevenzip_clang_scan_deps)
        set(sevenzip_modules_supported ON)
    endif()
endif()

option(SEVENZIP_ENABLE_DEPENDENCY_MODULES "Import named modules for dependency facades"
    ${sevenzip_modules_supported})
if(SEVENZIP_ENABLE_DEPENDENCY_MODULES)
    if(NOT sevenzip_modules_supported)
        message(FATAL_ERROR "Dependency modules require MSVC 19.34+, GCC 16+, or Clang 16+ with clang-scan-deps; GCC 14 fails to serialize this facade with sanitizers")
    endif()
endif()

function(sevenzip_add_dependency_facade dependency core)
    set(facade sevenzip_${dependency}_facade)
    if(SEVENZIP_ENABLE_DEPENDENCY_MODULES)
        string(TOUPPER "${dependency}" dependency_macro)
        add_library(${facade} STATIC)
        target_sources(${facade} PUBLIC FILE_SET CXX_MODULES
            FILES src/dependencies/${dependency}.cppm)
        target_link_libraries(${facade} PRIVATE ${core})
        target_compile_definitions(${facade} INTERFACE SEVENZIP_USE_${dependency_macro}_MODULE)
        set_target_properties(${facade} PROPERTIES
            AUTOMOC OFF AUTOUIC OFF POSITION_INDEPENDENT_CODE ON)
    else()
        add_library(${facade} INTERFACE)
        target_link_libraries(${facade} INTERFACE ${core})
    endif()
    target_include_directories(${facade} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")
endfunction()

sevenzip_add_dependency_facade(zlib sevenzip_zisofs_zlib)
sevenzip_add_dependency_facade(brotli sevenzip_brotli_core)
if(SEVENZIP_ENABLE_DEPENDENCY_MODULES)
    # Qt disables automatic scanning on qt_add_executable targets. This shared
    # source imports the facade in both the plugin and its Qt test executable.
    set_source_files_properties(src/squashfszlib.cpp
        src/CPP/7zip/Compress/BrotliDecoder.cpp src/CPP/7zip/Archive/BrotliHandler.cpp
        PROPERTIES CXX_SCAN_FOR_MODULES ON)
    sevenzip_configure_module_cache(sevenzip_squashfs)
    sevenzip_configure_module_cache(sevenzip_brotli)
    if(BUILD_TESTING)
        sevenzip_configure_module_cache(filesystem_zlib_tests)
    endif()
endif()
target_link_libraries(sevenzip_squashfs PRIVATE sevenzip_zlib_facade)
target_link_libraries(sevenzip_brotli PRIVATE sevenzip_brotli_facade)
if(BUILD_TESTING)
    target_link_libraries(filesystem_zlib_tests PRIVATE sevenzip_zlib_facade)
endif()
