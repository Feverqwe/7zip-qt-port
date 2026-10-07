option(SEVENZIP_COMPACT_TEST_BINARIES
    "Omit linked test debug information and incremental-link databases" OFF)

if(NOT BUILD_TESTING OR NOT SEVENZIP_COMPACT_TEST_BINARIES)
    return()
endif()

# Fresh runner snapshots do not reuse incremental links. Keep production
# binaries and compiler debug information available for diagnostic rebuilds.
get_property(sevenzip_compact_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(sevenzip_compact_target IN LISTS sevenzip_compact_targets)
    get_target_property(sevenzip_compact_type ${sevenzip_compact_target} TYPE)
    if(NOT sevenzip_compact_type STREQUAL "EXECUTABLE"
            OR sevenzip_compact_target STREQUAL "sevenzip_desktop")
        continue()
    endif()
    if(MSVC)
        target_link_options(${sevenzip_compact_target} PRIVATE
            /DEBUG:NONE /INCREMENTAL:NO /OPT:REF /OPT:NOICF)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_options(${sevenzip_compact_target} PRIVATE LINKER:--strip-debug)
    endif()
endforeach()
