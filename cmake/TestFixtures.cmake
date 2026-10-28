# Test-specific definitions stay on the consuming test bodies. These helpers
# have one implementation and do not consume per-scenario definitions.
function(sevenzip_add_test_fixture owner source)
    add_library(${owner} OBJECT ${source})
    target_include_directories(${owner} PRIVATE src src/CPP third_party/7zip/CPP tests)
    target_link_libraries(${owner} PRIVATE Qt6::Widgets sevenzip_common)
    set_target_properties(${owner} PROPERTIES AUTOMOC OFF AUTOUIC OFF)
endfunction()

sevenzip_add_test_fixture(sevenzip_layout_inspection tests/layout_inspection.cpp)
sevenzip_add_test_fixture(sevenzip_widget_dialog_fixtures tests/fixtures/widget_dialogs.cpp)
sevenzip_add_test_fixture(sevenzip_panel_load_fixture tests/panel_load_folder.cpp)
