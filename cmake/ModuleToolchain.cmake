include_guard(GLOBAL)

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
            "${Python3_EXECUTABLE};${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../scripts/lib/module_cache_launcher.py;${sevenzip_launcher}")
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
if(SEVENZIP_ENABLE_DEPENDENCY_MODULES AND NOT sevenzip_modules_supported)
    message(FATAL_ERROR "Dependency modules require MSVC 19.34+, GCC 16+, or Clang 16+ with clang-scan-deps; GCC 14 fails to serialize this facade with sanitizers")
endif()
