cmake_minimum_required(VERSION 3.21)

project(sevenzip_desktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

add_library(sevenzip_common STATIC third_party/7zip/CPP/Common/StringToInt.cpp
    third_party/7zip/CPP/Windows/System.cpp src/CPP/7zip/Common/MethodProps.cpp src/CPP/7zip/UI/Common/ZipRegistry.cpp
    src/CPP/7zip/UI/FileManager/ViewSettings.cpp src/CPP/7zip/UI/FileManager/ViewSettings.h)
set_target_properties(sevenzip_common PROPERTIES AUTOMOC OFF AUTOUIC OFF)
target_include_directories(sevenzip_common PUBLIC third_party/7zip/CPP)
target_link_libraries(sevenzip_common PUBLIC Qt6::Core)

add_library(sevenzip_panel STATIC src/CPP/7zip/UI/FileManager/App.cpp
    src/CPP/7zip/UI/FileManager/PanelListNotify.cpp src/CPP/7zip/UI/FileManager/Panel.h)
set_target_properties(sevenzip_panel PROPERTIES AUTOMOC OFF AUTOUIC OFF)
target_include_directories(sevenzip_panel PUBLIC src)
target_link_libraries(sevenzip_panel PUBLIC Qt6::Widgets sevenzip_common)

qt_add_executable(sevenzip_desktop
    src/main.cpp
    src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp
    src/createarchivedialog.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h
    src/combodialog.cpp src/combodialog.h
    src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h
    src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h
    src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp
    src/archivebackend.h src/archiveformats.h
    src/filesystemscanner.cpp
    src/filesystemscanner.h
    src/fileviewerdialog.cpp
    src/fileviewerdialog.h
    src/settingsdialog.cpp
    src/settingsdialog.h
    src/propertiesdialog.cpp
    src/propertiesdialog.h
    src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/volumedialog.cpp src/volumedialog.h
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
    qt_add_executable(panel_copy_info_tests tests/panel_copy_info_tests.cpp)
    target_link_libraries(panel_copy_info_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME panel_copy_info COMMAND panel_copy_info_tests)
    set_tests_properties(panel_copy_info PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(copy_dialog_tests tests/copy_dialog_tests.cpp tests/layout_inspection.cpp
        src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h)
    target_include_directories(copy_dialog_tests PRIVATE src tests)
    target_link_libraries(copy_dialog_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_panel)
    add_test(NAME copy_dialog COMMAND copy_dialog_tests)
    set_tests_properties(copy_dialog PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports" TIMEOUT 30)
    qt_add_executable(archive_backend_tests tests/archive_backend_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h)
    qt_add_executable(archive_mutation_tests tests/archive_mutation_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h)
    target_include_directories(archive_backend_tests PRIVATE src)
    target_include_directories(archive_mutation_tests PRIVATE src tests)
    target_link_libraries(archive_backend_tests PRIVATE Qt6::Test)
    target_link_libraries(archive_mutation_tests PRIVATE Qt6::Test)
    qt_add_executable(ui_layout_tests tests/ui_layout_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.cpp src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.cpp src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/volumedialog.cpp src/volumedialog.h)
    target_include_directories(ui_layout_tests PRIVATE src)
    target_link_libraries(ui_layout_tests PRIVATE Qt6::Widgets Qt6::Test)
    qt_add_executable(ui_state_tests tests/ui_state_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.cpp src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.cpp src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/volumedialog.cpp src/volumedialog.h)
    qt_add_executable(ui_operations_tests tests/ui_operations_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.cpp src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.cpp src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/volumedialog.cpp src/volumedialog.h)
    qt_add_executable(ui_navigation_tests tests/ui_navigation_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.cpp src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.cpp src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/volumedialog.cpp src/volumedialog.h)
    qt_add_executable(ui_archive_states_tests tests/ui_archive_states_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.cpp src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.cpp src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/volumedialog.cpp src/volumedialog.h)
    target_include_directories(ui_state_tests PRIVATE src)
    target_include_directories(ui_operations_tests PRIVATE src tests)
    target_include_directories(ui_navigation_tests PRIVATE src tests)
    target_include_directories(ui_archive_states_tests PRIVATE src tests)
    target_link_libraries(ui_state_tests PRIVATE Qt6::Widgets Qt6::Test)
    target_link_libraries(ui_operations_tests PRIVATE Qt6::Widgets Qt6::Test)
    target_link_libraries(ui_navigation_tests PRIVATE Qt6::Widgets Qt6::Test)
    target_link_libraries(ui_archive_states_tests PRIVATE Qt6::Widgets Qt6::Test)
    qt_add_executable(flat_view_tests tests/flat_view_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.cpp src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.cpp src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/volumedialog.cpp src/volumedialog.h)
    qt_add_executable(settings_tests tests/settings_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.cpp src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.cpp src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/volumedialog.cpp src/volumedialog.h)
    target_include_directories(settings_tests PRIVATE src)
    target_link_libraries(settings_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME settings COMMAND settings_tests)
    set_tests_properties(settings PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/settings-tests.ini")
    qt_add_executable(properties_tests tests/properties_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.cpp src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.cpp src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/volumedialog.cpp src/volumedialog.h)
    target_include_directories(properties_tests PRIVATE src)
    target_link_libraries(properties_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME properties COMMAND properties_tests)
    qt_add_executable(checksum_tests tests/checksum_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.cpp src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.cpp src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/volumedialog.cpp src/volumedialog.h)
    target_include_directories(checksum_tests PRIVATE src)
    target_link_libraries(checksum_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME checksum COMMAND checksum_tests)
    set_tests_properties(checksum PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/checksum-tests.ini")
    set_tests_properties(properties PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/properties-tests.ini")
    target_include_directories(flat_view_tests PRIVATE src)
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
    qt_add_executable(compression_tests tests/compression_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/createarchivedialog.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h)
    target_include_directories(compression_tests PRIVATE src)
    target_link_libraries(compression_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME compression COMMAND compression_tests)
    get_target_property(compression_order_sources compression_tests SOURCES)
    list(REMOVE_ITEM compression_order_sources tests/compression_tests.cpp)
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
    qt_add_executable(file_viewer_tests tests/file_viewer_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.cpp src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.cpp src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/volumedialog.cpp src/volumedialog.h)
    target_include_directories(file_viewer_tests PRIVATE src)
    target_link_libraries(file_viewer_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME file_viewer COMMAND file_viewer_tests)
    set_tests_properties(file_viewer PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/file-viewer-settings.ini")
    qt_add_executable(nested_archive_tests tests/nested_archive_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h src/extractdialog.h src/combodialog.cpp src/combodialog.h src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h src/copydialog.h src/settingsdialog.cpp src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/volumedialog.cpp src/volumedialog.h)
    target_include_directories(nested_archive_tests PRIVATE src)
    target_link_libraries(nested_archive_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME nested_archive COMMAND nested_archive_tests)
    set_tests_properties(nested_archive PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/nested-archive-settings.ini")
    qt_add_executable(file_transfer_tests tests/file_transfer_tests.cpp src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h)
    target_include_directories(file_transfer_tests PRIVATE src)
    target_link_libraries(file_transfer_tests PRIVATE Qt6::Test)
    add_test(NAME file_transfer COMMAND file_transfer_tests)
    get_target_property(panel_sort_sources ui_state_tests SOURCES)
    list(REMOVE_ITEM panel_sort_sources tests/ui_state_tests.cpp)
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
    target_link_libraries(archive_proxy_tests PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME archive_proxy COMMAND archive_proxy_tests)
    qt_add_executable(volume_operations_tests tests/volume_operations_tests.cpp src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/volumedialog.cpp src/volumedialog.h)
    target_include_directories(volume_operations_tests PRIVATE src)
    target_link_libraries(volume_operations_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME volume_operations COMMAND volume_operations_tests)
    set_tests_properties(volume_operations PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(ui_geometry_tests tests/ui_geometry_tests.cpp tests/layout_inspection.cpp
        ${compression_order_sources} src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/Common/Wildcard.cpp)
    target_include_directories(ui_geometry_tests PRIVATE src tests)
    target_link_libraries(ui_geometry_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME ui_geometry COMMAND ui_geometry_tests)
    set_tests_properties(ui_geometry PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    foreach(target IN ITEMS ui_layout_tests extract_dialog_tests compression_preferences_tests)
        target_sources(${target} PRIVATE tests/layout_inspection.cpp)
    endforeach()
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

foreach(target IN ITEMS sevenzip_desktop ui_layout_tests ui_state_tests ui_operations_tests
    ui_navigation_tests ui_archive_states_tests flat_view_tests settings_tests properties_tests
    checksum_tests file_viewer_tests nested_archive_tests panel_sort_tests combine_name_tests panel_key_tests)
    if(TARGET ${target})
        target_link_libraries(${target} PRIVATE sevenzip_panel)
    endif()
endforeach()
