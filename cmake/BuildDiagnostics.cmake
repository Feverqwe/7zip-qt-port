# Build modes select defaults; each diagnostic remains independently selectable.
set(SEVENZIP_BUILD_MODE "lite" CACHE STRING "Diagnostic defaults: lite, debug or sanitize")
set_property(CACHE SEVENZIP_BUILD_MODE PROPERTY STRINGS lite debug sanitize)
if(NOT SEVENZIP_BUILD_MODE MATCHES "^(lite|debug|sanitize)$")
    message(FATAL_ERROR "SEVENZIP_BUILD_MODE must be lite, debug or sanitize")
endif()
set(sevenzip_debug_symbols_default OFF)
set(sevenzip_sanitizers_default OFF)
if(NOT SEVENZIP_BUILD_MODE STREQUAL "lite")
    set(sevenzip_debug_symbols_default ON)
endif()
if(SEVENZIP_BUILD_MODE STREQUAL "sanitize")
    set(sevenzip_sanitizers_default ON)
endif()
option(SEVENZIP_DEBUG_SYMBOLS "Generate compiler and linker debug information"
    ${sevenzip_debug_symbols_default})
option(SEVENZIP_ENABLE_SANITIZERS "Instrument Debug and RelWithDebInfo builds"
    ${sevenzip_sanitizers_default})

if(NOT SEVENZIP_DEBUG_SYMBOLS)
    if(MSVC)
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "")
        add_link_options(/DEBUG:NONE /INCREMENTAL:NO)
    else()
        # Override configuration flags without changing Debug assertions or
        # introducing Release optimizations into the fast compilation mode.
        add_compile_options(-g0)
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            # Static startup objects and dependencies can carry their own DWARF.
            add_link_options(LINKER:--strip-debug)
        endif()
    endif()
elseif(MSVC)
    # Embedded information keeps compiler-cache hits usable for debugging.
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
    add_link_options(/DEBUG:FULL)
else()
    add_compile_options(-g)
endif()

if(SEVENZIP_ENABLE_SANITIZERS)
    if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"
            AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 22
            AND CMAKE_BUILD_TYPE MATCHES "^(Debug|RelWithDebInfo)$")
        # On an intentional compiler switch CMake still exposes the old cached
        # compiler ID until it restarts configuration. Check the actual driver.
        execute_process(COMMAND "${CMAKE_CXX_COMPILER}" --version
            OUTPUT_VARIABLE sevenzip_actual_compiler_version)
        if(sevenzip_actual_compiler_version MATCHES "Apple clang version")
            message(FATAL_ERROR "AppleClang 21 ASan retains stale globals after dlclose; use a consistent Homebrew LLVM 23+ C/C++/Objective-C++ toolchain for instrumented codec loading")
        endif()
    endif()
    if(MSVC)
        foreach(sevenzip_config DEBUG RELWITHDEBINFO)
            # MSVC's Debug /RTC checks cannot be combined with AddressSanitizer.
            foreach(sevenzip_language C CXX)
                string(REPLACE "/RTC1" "" CMAKE_${sevenzip_language}_FLAGS_${sevenzip_config}
                    "${CMAKE_${sevenzip_language}_FLAGS_${sevenzip_config}}")
                string(APPEND CMAKE_${sevenzip_language}_FLAGS_${sevenzip_config} " /fsanitize=address")
            endforeach()
            foreach(sevenzip_link_kind EXE SHARED MODULE)
                string(APPEND CMAKE_${sevenzip_link_kind}_LINKER_FLAGS_${sevenzip_config} " /INCREMENTAL:NO")
            endforeach()
        endforeach()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(AppleClang|Clang|GNU)$")
        foreach(sevenzip_config DEBUG RELWITHDEBINFO)
            foreach(sevenzip_language C CXX)
                string(APPEND CMAKE_${sevenzip_language}_FLAGS_${sevenzip_config} " -fsanitize=address,undefined -fno-omit-frame-pointer")
            endforeach()
            foreach(sevenzip_link_kind EXE SHARED MODULE)
                string(APPEND CMAKE_${sevenzip_link_kind}_LINKER_FLAGS_${sevenzip_config} " -fsanitize=address,undefined")
            endforeach()
        endforeach()
    else()
        message(FATAL_ERROR "Sanitizers are not configured for ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()
