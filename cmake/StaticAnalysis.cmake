include_guard(GLOBAL)

set(SEVENZIP_CLANG_TIDY_CONFIG "${CMAKE_CURRENT_LIST_DIR}/../.clang-tidy")
set(SEVENZIP_CLANG_TIDY_LAUNCHER "${CMAKE_CURRENT_LIST_DIR}/../scripts/lib/clang_tidy_launcher.py")
option(SEVENZIP_ENABLE_CLANG_TIDY "Run clang-tidy while compiling C++" OFF)
if(SEVENZIP_ENABLE_CLANG_TIDY)
    set(sevenzip_clang_tidy_names clang-tidy)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        string(REGEX MATCH "^[0-9]+" sevenzip_clang_major "${CMAKE_CXX_COMPILER_VERSION}")
        list(PREPEND sevenzip_clang_tidy_names "clang-tidy-${sevenzip_clang_major}")
    endif()
    find_program(SEVENZIP_CLANG_TIDY_EXECUTABLE
        NAMES ${sevenzip_clang_tidy_names} REQUIRED)
    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
    set(CMAKE_CXX_CLANG_TIDY
        "${Python3_EXECUTABLE};${SEVENZIP_CLANG_TIDY_LAUNCHER};${SEVENZIP_CLANG_TIDY_EXECUTABLE};--config-file=${SEVENZIP_CLANG_TIDY_CONFIG}")
    execute_process(COMMAND "${SEVENZIP_CLANG_TIDY_EXECUTABLE}" --version
        OUTPUT_VARIABLE sevenzip_clang_tidy_version OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    string(REGEX MATCH "LLVM version ([0-9]+\\.[0-9]+\\.[0-9]+)"
        sevenzip_clang_tidy_release_match "${sevenzip_clang_tidy_version}")
    set(sevenzip_clang_tidy_release "${CMAKE_MATCH_1}")
    if(sevenzip_clang_tidy_version MATCHES "LLVM version ([0-9]+)" AND CMAKE_MATCH_1 GREATER_EQUAL 23)
        list(APPEND CMAKE_CXX_CLANG_TIDY --extra-arg=-Wlifetime-safety-permissive)
    endif()
endif()

# Register after targets exist. Analysis runs before the compiler-cache lookup,
# so changed diagnostics are checked even when unchanged object bytes are cached.
function(sevenzip_track_analysis_inputs)
    if(NOT SEVENZIP_ENABLE_CLANG_TIDY)
        return()
    endif()
    get_property(sevenzip_analysis_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    set(sevenzip_analysis_sources)
    foreach(sevenzip_target IN LISTS sevenzip_analysis_targets)
        get_target_property(sevenzip_sources ${sevenzip_target} SOURCES)
        get_target_property(sevenzip_module_sets ${sevenzip_target} CXX_MODULE_SETS)
        if(sevenzip_module_sets)
            if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
                message(FATAL_ERROR
                    "Clang-Tidy cannot read ${CMAKE_CXX_COMPILER_ID} module BMIs. "
                    "Use a separate Clang build with matching Clang-Tidy; "
                    "MSVC IFC and GCC GCM are not Clang module formats.")
            endif()
            if(NOT sevenzip_clang_tidy_release VERSION_EQUAL CMAKE_CXX_COMPILER_VERSION)
                message(FATAL_ERROR
                    "Module analysis requires Clang-Tidy ${CMAKE_CXX_COMPILER_VERSION} "
                    "to match the Clang compiler; selected "
                    "${SEVENZIP_CLANG_TIDY_EXECUTABLE} reports ${sevenzip_clang_tidy_release}.")
            endif()
        endif()
        foreach(sevenzip_set IN LISTS sevenzip_module_sets)
            if(sevenzip_set STREQUAL "CXX_MODULES")
                get_target_property(sevenzip_modules ${sevenzip_target} CXX_MODULE_SET)
            else()
                get_target_property(sevenzip_modules ${sevenzip_target} CXX_MODULE_SET_${sevenzip_set})
            endif()
            if(sevenzip_modules)
                list(APPEND sevenzip_sources ${sevenzip_modules})
            endif()
        endforeach()
        foreach(sevenzip_source IN LISTS sevenzip_sources)
            if(NOT sevenzip_source MATCHES "^\\$<"
                    AND sevenzip_source MATCHES "\\.(cpp|cxx|cc|cppm|ixx)$")
                list(APPEND sevenzip_analysis_sources "${sevenzip_source}")
            endif()
        endforeach()
    endforeach()
    if(sevenzip_analysis_sources)
        list(REMOVE_DUPLICATES sevenzip_analysis_sources)
        set_property(SOURCE ${sevenzip_analysis_sources} APPEND PROPERTY
            OBJECT_DEPENDS "${SEVENZIP_CLANG_TIDY_CONFIG};${SEVENZIP_CLANG_TIDY_LAUNCHER}")
    endif()
endfunction()
