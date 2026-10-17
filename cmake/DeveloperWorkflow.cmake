# Export actual target paths, including bundle executables and target aliases
# whose CTest names differ. Generator expressions resolve after configuration.
if(NOT BUILD_TESTING)
    return()
endif()
get_property(sevenzip_workflow_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
set(sevenzip_workflow_manifest "")
foreach(sevenzip_workflow_target IN LISTS sevenzip_workflow_targets)
    get_target_property(sevenzip_workflow_type ${sevenzip_workflow_target} TYPE)
    if(sevenzip_workflow_type STREQUAL "EXECUTABLE")
        string(APPEND sevenzip_workflow_manifest
            "${sevenzip_workflow_target}\t$<TARGET_FILE:${sevenzip_workflow_target}>\n")
    endif()
endforeach()
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/verify-artifacts.tsv" CONTENT "${sevenzip_workflow_manifest}")
