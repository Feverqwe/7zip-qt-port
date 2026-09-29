cmake_minimum_required(VERSION 3.21)

project(sevenzip_desktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(property_name_resource "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/FileManager/PropertyName.rc")
set(extract_resource "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/GUI/Extract.rc")
set(resource_strings_header "${CMAKE_CURRENT_BINARY_DIR}/ResourceStrings.h")
add_custom_command(OUTPUT "${resource_strings_header}"
    COMMAND "${CMAKE_COMMAND}" "-DRESOURCE_FILES=${property_name_resource};${extract_resource}"
        "-DOUTPUT_FILE=${resource_strings_header}" -P "${CMAKE_CURRENT_SOURCE_DIR}/scripts/rc_strings.cmake"
    DEPENDS "${property_name_resource}" "${extract_resource}" scripts/rc_strings.cmake
    VERBATIM)

qt_standard_project_setup()

add_library(sevenzip_common STATIC third_party/7zip/CPP/Common/StringToInt.cpp
    third_party/7zip/CPP/Common/MyString.cpp third_party/7zip/CPP/Common/IntToString.cpp
    third_party/7zip/CPP/Common/MyWindows.cpp
    third_party/7zip/CPP/Common/StringConvert.cpp third_party/7zip/CPP/Common/UTFConvert.cpp
    third_party/7zip/CPP/Windows/System.cpp src/CPP/7zip/Common/MethodProps.cpp src/CPP/7zip/UI/Common/ZipRegistry.cpp
    third_party/7zip/CPP/Windows/PropVariant.cpp third_party/7zip/CPP/Windows/PropVariantConv.cpp
    src/CPP/7zip/UI/FileManager/ClassDefs.cpp
    third_party/7zip/CPP/7zip/Common/PropId.cpp
    src/CPP/7zip/UI/FileManager/ViewSettings.cpp src/CPP/7zip/UI/FileManager/ViewSettings.h
    src/CPP/7zip/UI/FileManager/RegistryUtils.cpp src/CPP/7zip/UI/FileManager/RegistryUtils.h)
set_target_properties(sevenzip_common PROPERTIES AUTOMOC OFF AUTOUIC OFF)
target_include_directories(sevenzip_common PUBLIC third_party/7zip/CPP)
target_link_libraries(sevenzip_common PUBLIC Qt6::Core)
if(WIN32)
    target_link_libraries(sevenzip_common PUBLIC oleaut32 uuid)
endif()

add_library(sevenzip_panel STATIC src/CPP/7zip/UI/FileManager/App.cpp
    src/qtpanellist.cpp src/qtpanellist.h
    src/qtpanelfolder.cpp src/qtpanelfolder.h
    src/CPP/7zip/UI/FileManager/App.h src/CPP/7zip/UI/FileManager/Panel.cpp
    src/CPP/7zip/UI/FileManager/SettingsPage.cpp src/CPP/7zip/UI/FileManager/SettingsPage.h
    src/CPP/7zip/UI/FileManager/PanelFolderChange.cpp
    src/CPP/7zip/UI/FileManager/PanelMenu.cpp
    src/CPP/7zip/UI/FileManager/PropertyName.cpp src/CPP/7zip/UI/FileManager/PropertyName.h
    src/CPP/7zip/UI/FileManager/ExtractCallback.cpp src/CPP/7zip/UI/FileManager/ExtractCallback.h
    src/CPP/7zip/UI/Common/OpenArchive.cpp src/CPP/7zip/UI/Common/OpenArchive.h
    src/CPP/7zip/UI/FileManager/LangUtils.h src/CPP/Windows/ResourceString.cpp src/CPP/Windows/ResourceString.h
    "${resource_strings_header}"
    src/CPP/7zip/UI/FileManager/PanelItems.cpp
    src/qtlistview.cpp src/qtlistview.h
    src/CPP/7zip/UI/FileManager/AppState.h
    src/CPP/7zip/UI/FileManager/ListViewDialog.cpp src/CPP/7zip/UI/FileManager/ListViewDialog.h
    src/CPP/7zip/UI/FileManager/EditDialog.cpp src/CPP/7zip/UI/FileManager/EditDialog.h
    src/CPP/7zip/UI/FileManager/ComboDialog.cpp src/CPP/7zip/UI/FileManager/ComboDialog.h
    src/CPP/7zip/UI/FileManager/PasswordDialog.cpp src/CPP/7zip/UI/FileManager/PasswordDialog.h
    src/CPP/7zip/UI/FileManager/BrowseDialog.cpp src/CPP/7zip/UI/FileManager/BrowseDialog.h
    src/CPP/7zip/UI/FileManager/PanelListNotify.cpp src/CPP/7zip/UI/FileManager/Panel.h)
set_target_properties(sevenzip_panel PROPERTIES AUTOUIC OFF)
target_include_directories(sevenzip_panel PUBLIC src)
target_include_directories(sevenzip_panel PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(sevenzip_panel PUBLIC Qt6::Widgets sevenzip_common)

add_library(sevenzip_filesystem STATIC
    src/CPP/7zip/UI/FileManager/FSFolder.cpp
    src/CPP/7zip/UI/FileManager/FSDrives.cpp src/CPP/7zip/UI/FileManager/FSDrives.h
    src/CPP/Windows/FileDir.cpp src/CPP/Windows/FileDir.h
    src/CPP/Windows/FileName.cpp
    src/CPP/Windows/FileLink.cpp
    third_party/7zip/CPP/7zip/UI/Common/PropIDUtils.cpp
    third_party/7zip/CPP/Windows/FileFind.cpp
    third_party/7zip/CPP/Windows/TimeUtils.cpp
    src/filedeleteoperation.cpp src/filedeleteoperation.h)
if(WIN32)
    target_sources(sevenzip_filesystem PRIVATE third_party/7zip/CPP/Windows/FileIO.cpp)
    target_sources(sevenzip_filesystem PRIVATE third_party/7zip/CPP/Windows/FileSystem.cpp)
endif()
target_include_directories(sevenzip_filesystem PUBLIC src)
target_include_directories(sevenzip_filesystem PRIVATE third_party/7zip/CPP/Windows)
target_link_libraries(sevenzip_filesystem PUBLIC Qt6::Core sevenzip_common)
target_link_libraries(sevenzip_panel PUBLIC sevenzip_filesystem)

add_library(sevenzip_location STATIC src/qtlocationfolder.cpp src/qtlocationfolder.h
    src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/AgentProxy.cpp)
target_include_directories(sevenzip_location PUBLIC src)
target_link_libraries(sevenzip_location PUBLIC sevenzip_filesystem sevenzip_panel)

add_library(sevenzip_folder_loader STATIC
    src/filesystemfolderloader.cpp src/filesystemfolderloader.h)
target_include_directories(sevenzip_folder_loader PUBLIC src)
target_link_libraries(sevenzip_folder_loader PUBLIC sevenzip_location Qt6::Core)

qt_add_executable(sevenzip_desktop
    src/main.cpp
    src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp
    src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h
    src/combodialog.h
    src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h
    src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h
    src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp
    src/archivebackend.h src/archiveformats.h
    src/filesystemscanner.cpp
    src/filesystemscanner.h
    src/fileviewerdialog.cpp
    src/fileviewerdialog.h
    src/settingsdialog.h
    src/propertiesdialog.cpp
    src/propertiesdialog.h
    src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h
)

target_include_directories(sevenzip_desktop PRIVATE src)
target_link_libraries(sevenzip_desktop PRIVATE Qt6::Widgets)

set_target_properties(sevenzip_desktop PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(CTest)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(password_dialog_tests tests/password_dialog_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(password_dialog_tests PRIVATE tests)
    target_link_libraries(password_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME password_dialog COMMAND password_dialog_tests)
    set_tests_properties(password_dialog PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(edit_dialog_tests tests/edit_dialog_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(edit_dialog_tests PRIVATE tests)
    target_link_libraries(edit_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME edit_dialog COMMAND edit_dialog_tests)
    set_tests_properties(edit_dialog PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(list_view_dialog_tests tests/list_view_dialog_tests.cpp)
    target_link_libraries(list_view_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME list_view_dialog COMMAND list_view_dialog_tests)
    set_tests_properties(list_view_dialog PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(list_activation_tests tests/list_activation_tests.cpp)
    target_link_libraries(list_activation_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME list_activation COMMAND list_activation_tests)
    set_tests_properties(list_activation PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(folder_history_tests tests/folder_history_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(folder_history_tests PRIVATE tests)
    target_link_libraries(folder_history_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME folder_history COMMAND folder_history_tests)
    set_tests_properties(folder_history PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
    qt_add_executable(app_state_tests tests/app_state_tests.cpp)
    target_link_libraries(app_state_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME app_state COMMAND app_state_tests)
    qt_add_executable(folder_contract_tests tests/folder_contract_tests.cpp)
    target_link_libraries(folder_contract_tests PRIVATE sevenzip_common Qt6::Test)
    add_test(NAME folder_contract COMMAND folder_contract_tests)
    qt_add_executable(panel_folder_tests tests/panel_folder_tests.cpp)
    target_link_libraries(panel_folder_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME panel_folder COMMAND panel_folder_tests)
    set_tests_properties(panel_folder PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_test(NAME ui_parity_tool COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_parity_tests.py)
    add_test(NAME ui_parity_copy COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_ui_parity.py compare
        --reference ${CMAKE_CURRENT_SOURCE_DIR}/docs/ui-reference/copy-dialog.reference.json
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/docs/ui-reference/copy-dialog.contract.json
        --reports ${CMAKE_BINARY_DIR}/ui-reports
        --output ${CMAKE_BINARY_DIR}/ui-reports/copy-parity.json)
    set_tests_properties(ui_parity_copy PROPERTIES FIXTURES_REQUIRED ui_reference_copy)
    add_test(NAME ui_parity_password COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_ui_parity.py compare
        --reference ${CMAKE_CURRENT_SOURCE_DIR}/docs/ui-reference/password-dialog.reference.json
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/docs/ui-reference/password-dialog.contract.json
        --reports ${CMAKE_BINARY_DIR}/ui-reports
        --output ${CMAKE_BINARY_DIR}/ui-reports/password-parity.json)
    set_tests_properties(password_dialog PROPERTIES FIXTURES_SETUP ui_reference_password)
    set_tests_properties(ui_parity_password PROPERTIES FIXTURES_REQUIRED ui_reference_password)
    qt_add_executable(combo_dialog_tests tests/combo_dialog_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(combo_dialog_tests PRIVATE tests)
    target_link_libraries(combo_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME combo_dialog COMMAND combo_dialog_tests)
    set_tests_properties(combo_dialog PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(volume_dialog_tests tests/volume_dialog_tests.cpp tests/layout_inspection.cpp
        src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitUtils.cpp
        src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp)
    target_include_directories(volume_dialog_tests PRIVATE src tests)
    target_link_libraries(volume_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME volume_dialog COMMAND volume_dialog_tests)
    set_tests_properties(volume_dialog PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(filesystem_operations_tests tests/filesystem_operations_tests.cpp)
    target_link_libraries(filesystem_operations_tests PRIVATE sevenzip_filesystem Qt6::Test)
    add_test(NAME filesystem_operations COMMAND filesystem_operations_tests)
    qt_add_executable(file_find_tests tests/file_find_tests.cpp)
    target_link_libraries(file_find_tests PRIVATE sevenzip_filesystem Qt6::Test)
    add_test(NAME file_find COMMAND file_find_tests)
    qt_add_executable(fs_folder_tests tests/fs_folder_tests.cpp)
    target_link_libraries(fs_folder_tests PRIVATE sevenzip_filesystem Qt6::Test)
    add_test(NAME fs_folder COMMAND fs_folder_tests)
    qt_add_executable(location_folder_tests tests/location_folder_tests.cpp)
    target_link_libraries(location_folder_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME location_folder COMMAND location_folder_tests)
    qt_add_executable(filesystem_scanner_tests tests/filesystem_scanner_tests.cpp
        src/filesystemscanner.cpp src/filesystemscanner.h)
    target_include_directories(filesystem_scanner_tests PRIVATE src)
    target_link_libraries(filesystem_scanner_tests PRIVATE sevenzip_folder_loader Qt6::Test)
    add_test(NAME filesystem_scanner COMMAND filesystem_scanner_tests)
    qt_add_executable(panel_copy_info_tests tests/panel_copy_info_tests.cpp)
    target_link_libraries(panel_copy_info_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME panel_copy_info COMMAND panel_copy_info_tests)
    set_tests_properties(panel_copy_info PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(copy_dialog_tests tests/copy_dialog_tests.cpp tests/layout_inspection.cpp
        src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h)
    target_include_directories(copy_dialog_tests PRIVATE src tests)
    target_link_libraries(copy_dialog_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_panel)
    add_test(NAME copy_dialog COMMAND copy_dialog_tests)
    set_tests_properties(copy_dialog PROPERTIES FIXTURES_SETUP ui_reference_copy)
    set_tests_properties(copy_dialog PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports" TIMEOUT 30)
    qt_add_executable(archive_backend_tests tests/archive_backend_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h)
    qt_add_executable(archive_mutation_tests tests/archive_mutation_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h)
    target_include_directories(archive_backend_tests PRIVATE src)
    target_include_directories(archive_mutation_tests PRIVATE src tests)
    target_link_libraries(archive_backend_tests PRIVATE Qt6::Test)
    target_link_libraries(archive_mutation_tests PRIVATE Qt6::Test)
    qt_add_executable(ui_layout_tests tests/ui_layout_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(ui_layout_tests PRIVATE src)
    target_link_libraries(ui_layout_tests PRIVATE Qt6::Widgets Qt6::Test)
    qt_add_executable(ui_state_tests tests/ui_state_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    qt_add_executable(ui_operations_tests tests/ui_operations_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    qt_add_executable(ui_navigation_tests tests/ui_navigation_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    qt_add_executable(ui_archive_states_tests tests/ui_archive_states_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(ui_state_tests PRIVATE src)
    target_include_directories(ui_operations_tests PRIVATE src tests)
    target_include_directories(ui_navigation_tests PRIVATE src tests)
    target_include_directories(ui_archive_states_tests PRIVATE src tests)
    target_link_libraries(ui_state_tests PRIVATE Qt6::Widgets Qt6::Test)
    target_link_libraries(ui_operations_tests PRIVATE Qt6::Widgets Qt6::Test)
    target_link_libraries(ui_navigation_tests PRIVATE Qt6::Widgets Qt6::Test)
    target_link_libraries(ui_archive_states_tests PRIVATE Qt6::Widgets Qt6::Test)
    qt_add_executable(flat_view_tests tests/flat_view_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    qt_add_executable(settings_tests tests/settings_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(settings_tests PRIVATE src)
    target_link_libraries(settings_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME settings COMMAND settings_tests)
    set_tests_properties(settings PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/settings-tests.ini")
    qt_add_executable(properties_tests tests/properties_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(properties_tests PRIVATE src)
    target_link_libraries(properties_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME properties COMMAND properties_tests)
    qt_add_executable(checksum_tests tests/checksum_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(checksum_tests PRIVATE src)
    target_link_libraries(checksum_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME checksum COMMAND checksum_tests)
    set_tests_properties(checksum PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/checksum-tests.ini")
    set_tests_properties(properties PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/properties-tests.ini")
    target_include_directories(flat_view_tests PRIVATE src)
    target_sources(flat_view_tests PRIVATE tests/layout_inspection.cpp)
    target_link_libraries(flat_view_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME flat_view COMMAND flat_view_tests)
    set_tests_properties(flat_view PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/flat-view-settings.ini"
    )
    add_test(NAME ui_state COMMAND ui_state_tests)
    add_test(NAME ui_operations COMMAND ui_operations_tests)
    add_test(NAME ui_navigation COMMAND ui_navigation_tests)
    add_test(NAME ui_archive_states COMMAND ui_archive_states_tests)
    foreach(suite ui_operations ui_navigation ui_archive_states)
        set_tests_properties(${suite} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/${suite}-settings.ini")
    endforeach()
    set_property(TEST ui_navigation APPEND PROPERTY ENVIRONMENT "SEVENZIP_SCREENSHOT_DIR=${CMAKE_BINARY_DIR}/ui-screenshots")
    set_tests_properties(ui_state PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SCREENSHOT_DIR=${CMAKE_BINARY_DIR}/ui-screenshots;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/ui-state-settings.ini"
    )
    add_test(NAME ui_layout COMMAND ui_layout_tests)
    set_tests_properties(ui_layout PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SCREENSHOT_DIR=${CMAKE_BINARY_DIR}/ui-screenshots;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/ui-layout-settings.ini"
    )
    add_test(NAME archive_backend COMMAND archive_backend_tests)
    add_test(NAME archive_mutation COMMAND archive_mutation_tests)
    qt_add_executable(extract_dialog_tests tests/extract_dialog_tests.cpp src/CPP/Common/Wildcard.cpp
        src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h
        src/CPP/7zip/UI/GUI/CompressDialog.cpp src/archivebackend.cpp
        src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp)
    target_include_directories(extract_dialog_tests PRIVATE src)
    target_link_libraries(extract_dialog_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME extract_dialog COMMAND extract_dialog_tests)
    set_tests_properties(extract_dialog PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
    get_target_property(split_destination_sources extract_dialog_tests SOURCES)
    list(REMOVE_ITEM split_destination_sources tests/extract_dialog_tests.cpp)
    qt_add_executable(split_destination_tests tests/split_destination_tests.cpp ${split_destination_sources})
    target_include_directories(split_destination_tests PRIVATE src)
    target_link_libraries(split_destination_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME split_destination COMMAND split_destination_tests)
    set_tests_properties(split_destination PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
    qt_add_executable(extraction_options_tests tests/extraction_options_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h)
    target_include_directories(extraction_options_tests PRIVATE src)
    target_link_libraries(extraction_options_tests PRIVATE Qt6::Test)
    add_test(NAME extraction_options COMMAND extraction_options_tests)
    qt_add_executable(compression_tests tests/compression_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h)
    target_include_directories(compression_tests PRIVATE src)
    target_link_libraries(compression_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME compression COMMAND compression_tests)
    get_target_property(compression_order_sources compression_tests SOURCES)
    list(REMOVE_ITEM compression_order_sources tests/compression_tests.cpp)
    qt_add_executable(compression_path_tests tests/compression_path_tests.cpp tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(compression_path_tests PRIVATE src tests)
    target_link_libraries(compression_path_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME compression_path COMMAND compression_path_tests)
    set_tests_properties(compression_path PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(compression_encryption_tests tests/compression_encryption_tests.cpp tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(compression_encryption_tests PRIVATE src tests)
    target_link_libraries(compression_encryption_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME compression_encryption COMMAND compression_encryption_tests)
    set_tests_properties(compression_encryption PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(compression_order_tests tests/compression_order_tests.cpp ${compression_order_sources})
    target_include_directories(compression_order_tests PRIVATE src)
    target_link_libraries(compression_order_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME compression_order COMMAND compression_order_tests)
    qt_add_executable(compression_dictionary_tests tests/compression_dictionary_tests.cpp ${compression_order_sources})
    target_include_directories(compression_dictionary_tests PRIVATE src)
    target_link_libraries(compression_dictionary_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME compression_dictionary COMMAND compression_dictionary_tests)
    set_tests_properties(compression_dictionary PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(compression_memory_tests tests/compression_memory_tests.cpp ${compression_order_sources})
    target_include_directories(compression_memory_tests PRIVATE src)
    target_link_libraries(compression_memory_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME compression_memory COMMAND compression_memory_tests)
    set_tests_properties(compression_memory PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(memory_limit_tests tests/memory_limit_tests.cpp)
    target_include_directories(memory_limit_tests PRIVATE src)
    target_link_libraries(memory_limit_tests PRIVATE Qt6::Test sevenzip_common)
    add_test(NAME memory_limit COMMAND memory_limit_tests)
    qt_add_executable(compression_limit_tests tests/compression_limit_tests.cpp ${compression_order_sources})
    target_include_directories(compression_limit_tests PRIVATE src)
    target_link_libraries(compression_limit_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME compression_limit COMMAND compression_limit_tests)
    set_tests_properties(compression_limit PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(compression_preferences_tests tests/compression_preferences_tests.cpp ${compression_order_sources})
    target_include_directories(compression_preferences_tests PRIVATE src)
    target_link_libraries(compression_preferences_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME compression_preferences COMMAND compression_preferences_tests)
    set_tests_properties(compression_preferences PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(compression_resource_tests tests/compression_resource_tests.cpp ${compression_order_sources})
    target_include_directories(compression_resource_tests PRIVATE src)
    target_link_libraries(compression_resource_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME compression_resources COMMAND compression_resource_tests)
    set_tests_properties(compression_resources PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(compression_solid_tests tests/compression_solid_tests.cpp ${compression_order_sources})
    target_include_directories(compression_solid_tests PRIVATE src)
    target_link_libraries(compression_solid_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME compression_solid COMMAND compression_solid_tests)
    set_tests_properties(compression_solid PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    set_tests_properties(compression_order PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    set_tests_properties(compression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    foreach(suite compression compression_order compression_dictionary compression_memory compression_resources compression_solid compression_limit)
        set_property(TEST ${suite} APPEND PROPERTY ENVIRONMENT "SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/${suite}-settings.ini")
    endforeach()
    qt_add_executable(file_viewer_tests tests/file_viewer_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(file_viewer_tests PRIVATE src)
    target_link_libraries(file_viewer_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME file_viewer COMMAND file_viewer_tests)
    set_tests_properties(file_viewer PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/file-viewer-settings.ini")
    qt_add_executable(nested_archive_tests tests/nested_archive_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(nested_archive_tests PRIVATE src)
    target_link_libraries(nested_archive_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME nested_archive COMMAND nested_archive_tests)
    set_tests_properties(nested_archive PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/nested-archive-settings.ini")
    qt_add_executable(file_transfer_tests tests/file_transfer_tests.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h)
    target_include_directories(file_transfer_tests PRIVATE src)
    target_link_libraries(file_transfer_tests PRIVATE Qt6::Test sevenzip_common)
    add_test(NAME file_transfer COMMAND file_transfer_tests)
    get_target_property(panel_sort_sources ui_state_tests SOURCES)
    list(REMOVE_ITEM panel_sort_sources tests/ui_state_tests.cpp)
    qt_add_executable(bookmark_tests tests/bookmark_tests.cpp ${panel_sort_sources})
    target_include_directories(bookmark_tests PRIVATE src tests)
    target_link_libraries(bookmark_tests PRIVATE sevenzip_panel sevenzip_filesystem Qt6::Test)
    target_sources(bookmark_tests PRIVATE src/mainwindow_filedelete.cpp)
    add_test(NAME bookmarks COMMAND bookmark_tests)
    set_tests_properties(bookmarks PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/bookmark-settings.ini")
    qt_add_executable(panel_focus_tests tests/panel_focus_tests.cpp ${panel_sort_sources})
    target_include_directories(panel_focus_tests PRIVATE src tests)
    target_link_libraries(panel_focus_tests PRIVATE sevenzip_panel sevenzip_filesystem Qt6::Test)
    target_sources(panel_focus_tests PRIVATE src/mainwindow_filedelete.cpp)
    add_test(NAME panel_focus COMMAND panel_focus_tests)
    set_tests_properties(panel_focus PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-focus-settings.ini")
    qt_add_executable(panel_selected_state_tests tests/panel_selected_state_tests.cpp ${panel_sort_sources})
    target_include_directories(panel_selected_state_tests PRIVATE src tests)
    target_link_libraries(panel_selected_state_tests PRIVATE sevenzip_panel sevenzip_filesystem Qt6::Test)
    target_sources(panel_selected_state_tests PRIVATE src/mainwindow_filedelete.cpp)
    target_sources(panel_selected_state_tests PRIVATE tests/layout_inspection.cpp)
    add_test(NAME panel_selected_state COMMAND panel_selected_state_tests)
    set_tests_properties(panel_selected_state PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-selected-state-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_runtime_tests tests/panel_runtime_tests.cpp ${panel_sort_sources})
    target_include_directories(panel_runtime_tests PRIVATE src tests)
    target_link_libraries(panel_runtime_tests PRIVATE sevenzip_panel sevenzip_filesystem Qt6::Test)
    target_sources(panel_runtime_tests PRIVATE src/mainwindow_filedelete.cpp)
    target_sources(panel_runtime_tests PRIVATE tests/layout_inspection.cpp)
    add_test(NAME panel_runtime COMMAND panel_runtime_tests)
    set_tests_properties(panel_runtime PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-runtime-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(properties_runtime_tests tests/properties_runtime_tests.cpp ${panel_sort_sources})
    target_include_directories(properties_runtime_tests PRIVATE src tests)
    target_link_libraries(properties_runtime_tests PRIVATE sevenzip_panel sevenzip_filesystem Qt6::Test)
    target_sources(properties_runtime_tests PRIVATE src/mainwindow_filedelete.cpp tests/layout_inspection.cpp)
    add_test(NAME properties_runtime COMMAND properties_runtime_tests)
    set_tests_properties(properties_runtime PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/properties-runtime-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(property_formatting_tests tests/property_formatting_tests.cpp)
    target_link_libraries(property_formatting_tests PRIVATE sevenzip_filesystem Qt6::Test)
    add_test(NAME property_formatting COMMAND property_formatting_tests)

    qt_add_executable(property_classification_tests tests/property_classification_tests.cpp)
    target_link_libraries(property_classification_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME property_classification COMMAND property_classification_tests)

    qt_add_executable(property_name_tests tests/property_name_tests.cpp)
    target_link_libraries(property_name_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME property_name COMMAND property_name_tests)

    qt_add_executable(archive_error_properties_tests tests/archive_error_properties_tests.cpp)
    target_link_libraries(archive_error_properties_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME archive_error_properties COMMAND archive_error_properties_tests)

    qt_add_executable(panel_property_tests tests/panel_property_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(panel_property_tests PRIVATE tests)
    target_link_libraries(panel_property_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME panel_property COMMAND panel_property_tests)
    set_tests_properties(panel_property PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")

    qt_add_executable(archive_property_catalog_tests tests/archive_property_catalog_tests.cpp)
    target_link_libraries(archive_property_catalog_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME archive_property_catalog COMMAND archive_property_catalog_tests)
    add_test(NAME resource_strings_generator COMMAND "${CMAKE_COMMAND}"
        "-DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/resource-strings-tests"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/rc_strings_tests.cmake")
    qt_add_executable(volume_workflow_tests tests/volume_workflow_tests.cpp ${panel_sort_sources})
    target_include_directories(volume_workflow_tests PRIVATE src tests)
    target_link_libraries(volume_workflow_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME volume_workflow COMMAND volume_workflow_tests)
    set_tests_properties(volume_workflow PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/volume-workflow-settings.ini")
    qt_add_executable(panel_sort_tests tests/panel_sort_tests.cpp ${panel_sort_sources})
    target_include_directories(panel_sort_tests PRIVATE src)
    target_link_libraries(panel_sort_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME panel_sort COMMAND panel_sort_tests)
    qt_add_executable(combine_name_tests tests/combine_name_tests.cpp ${panel_sort_sources})
    target_include_directories(combine_name_tests PRIVATE src)
    target_link_libraries(combine_name_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME combine_names COMMAND combine_name_tests)
    set_tests_properties(combine_names PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/combine-name-settings.ini")
    qt_add_executable(panel_key_tests tests/panel_key_tests.cpp ${panel_sort_sources})
    target_include_directories(panel_key_tests PRIVATE src)
    target_link_libraries(panel_key_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME panel_key COMMAND panel_key_tests)
    set_tests_properties(panel_key PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-key-settings.ini")
    set_tests_properties(panel_sort PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-sort-settings.ini;SEVENZIP_SCREENSHOT_DIR=${CMAKE_BINARY_DIR}/ui-screenshots")
    qt_add_executable(panel_selection_tests tests/panel_selection_tests.cpp
        src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h
        src/CPP/Common/Wildcard.cpp)
    target_include_directories(panel_selection_tests PRIVATE src)
    target_link_libraries(panel_selection_tests PRIVATE Qt6::Test)
    add_test(NAME panel_selection COMMAND panel_selection_tests)
    qt_add_executable(archive_proxy_tests tests/archive_proxy_tests.cpp src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/Agent.cpp)
    target_include_directories(archive_proxy_tests PRIVATE src)
    target_link_libraries(archive_proxy_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME archive_proxy COMMAND archive_proxy_tests)
    qt_add_executable(agent_folder_tests tests/agent_folder_tests.cpp src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/Agent.cpp)
    target_include_directories(agent_folder_tests PRIVATE src)
    target_link_libraries(agent_folder_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME agent_folder COMMAND agent_folder_tests)
    qt_add_executable(volume_operations_tests tests/volume_operations_tests.cpp src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(volume_operations_tests PRIVATE src)
    target_link_libraries(volume_operations_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME volume_operations COMMAND volume_operations_tests)
    set_tests_properties(volume_operations PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(ui_geometry_tests tests/ui_geometry_tests.cpp tests/layout_inspection.cpp
        ${compression_order_sources} src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/Common/Wildcard.cpp)
    target_include_directories(ui_geometry_tests PRIVATE src tests)
    target_link_libraries(ui_geometry_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME ui_geometry COMMAND ui_geometry_tests)
    set_tests_properties(ui_geometry PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    foreach(target IN ITEMS ui_layout_tests extract_dialog_tests compression_preferences_tests compression_tests)
        target_sources(${target} PRIVATE tests/layout_inspection.cpp)
    endforeach()
    target_sources(properties_tests PRIVATE tests/layout_inspection.cpp)
    qt_add_executable(fm_settings_tests tests/fm_settings_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(fm_settings_tests PRIVATE src tests)
    target_link_libraries(fm_settings_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME fm_settings COMMAND fm_settings_tests)
    set_tests_properties(fm_settings PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_property(TEST properties APPEND PROPERTY ENVIRONMENT
        "SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    foreach(suite IN ITEMS list_view_dialog folder_history)
        set_property(TEST ${suite} APPEND PROPERTY ENVIRONMENT
            "SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/${suite}-settings.ini")
    endforeach()
    find_program(SEVENZIP_TEST_SH NAMES sh)
    if(SEVENZIP_TEST_SH)
        add_test(NAME project_memory_setup COMMAND ${SEVENZIP_TEST_SH}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/project_memory_setup_test.sh)
    endif()
    add_test(NAME application_starts COMMAND sevenzip_desktop --smoke-test)
    set_tests_properties(application_starts PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

foreach(target IN ITEMS sevenzip_desktop archive_proxy_tests archive_backend_tests archive_mutation_tests ui_layout_tests ui_state_tests ui_operations_tests ui_navigation_tests ui_archive_states_tests flat_view_tests settings_tests properties_tests checksum_tests extraction_options_tests compression_tests file_viewer_tests nested_archive_tests volume_operations_tests)
    if(TARGET ${target})
        target_link_libraries(${target} PRIVATE sevenzip_common)
    endif()
endforeach()

# Window runtime uses the authoritative source-owned archive location factory.
if(TARGET flat_view_tests)
    set_property(TEST flat_view APPEND PROPERTY ENVIRONMENT
        "SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
endif()
foreach(target IN ITEMS sevenzip_desktop ui_layout_tests ui_state_tests ui_operations_tests
    ui_navigation_tests ui_archive_states_tests flat_view_tests settings_tests properties_tests
    checksum_tests file_viewer_tests nested_archive_tests panel_sort_tests combine_name_tests
    panel_key_tests volume_workflow_tests bookmark_tests panel_focus_tests panel_selected_state_tests panel_runtime_tests properties_runtime_tests)
    if(TARGET ${target})
        target_link_libraries(${target} PRIVATE sevenzip_location sevenzip_folder_loader)
    endif()
endforeach()

foreach(target IN ITEMS sevenzip_desktop ui_layout_tests ui_state_tests ui_operations_tests
    ui_navigation_tests ui_archive_states_tests flat_view_tests settings_tests properties_tests
    checksum_tests file_viewer_tests nested_archive_tests panel_sort_tests combine_name_tests panel_key_tests volume_workflow_tests)
    if(TARGET ${target})
        target_link_libraries(${target} PRIVATE sevenzip_panel)
        target_link_libraries(${target} PRIVATE sevenzip_filesystem)
        target_sources(${target} PRIVATE src/mainwindow_filedelete.cpp)
    endif()
endforeach()
