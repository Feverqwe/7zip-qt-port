# Test-specific definitions stay on the consuming test bodies. These helpers
# have one implementation and do not consume per-scenario definitions.
function(sevenzip_add_test_fixture owner source)
    add_library(${owner} OBJECT ${source})
    target_include_directories(${owner} PRIVATE src ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP tests)
    target_link_libraries(${owner} PRIVATE Qt6::Widgets sevenzip_common)
    set_target_properties(${owner} PROPERTIES AUTOMOC OFF AUTOUIC OFF)
endfunction()

sevenzip_add_test_fixture(sevenzip_layout_inspection tests/ui/support/layout_inspection.cpp)
sevenzip_add_test_fixture(sevenzip_widget_dialog_fixtures tests/support/widget_dialogs.cpp)
sevenzip_add_test_fixture(sevenzip_panel_load_fixture tests/filemanager/support/panel_load_folder.cpp)

# Test headers use owner-qualified paths; those paths are private to test targets.
function(sevenzip_set_test_source_roots)
    get_property(owners DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(owner IN LISTS owners)
        get_target_property(sources ${owner} SOURCES)
        foreach(source IN LISTS sources)
            if(source MATCHES "^tests/" OR source MATCHES "^${CMAKE_CURRENT_SOURCE_DIR}/tests/")
                target_include_directories(${owner} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
                break()
            endif()
        endforeach()
    endforeach()
endfunction()
