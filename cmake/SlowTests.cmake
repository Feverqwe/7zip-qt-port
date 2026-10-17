if(NOT BUILD_TESTING)
    return()
endif()

# These matrix suites exceeded 30 seconds in the recorded local runs.
# Keep them available for explicit validation without extending every gate.
foreach(sevenzip_slow_test IN ITEMS
        native_rename_2 native_rename_301
        native_external_2 native_external_301
        native_renderer_failure native_renderer_copy
        native_nested_stream native_nested_memory native_nested_disk
        interarchive_refresh panel_creation_publication copy_archive_refresh)
    if(TEST ${sevenzip_slow_test})
        set_property(TEST ${sevenzip_slow_test} APPEND PROPERTY LABELS slow)
    endif()
endforeach()
