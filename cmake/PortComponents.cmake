# Production sources have one owner. Consumers link component targets directly.
function(sevenzip_add_port_component owner header)
    add_library(${owner} OBJECT ${ARGN} ${header})
    target_compile_features(${owner} PUBLIC cxx_std_23)
    target_include_directories(${owner} PRIVATE src src/CPP third_party/7zip/CPP)
    target_link_libraries(${owner} PRIVATE Qt6::Widgets sevenzip_common)
    set_target_properties(${owner} PROPERTIES AUTOUIC OFF)
endfunction()

sevenzip_add_port_component(sevenzip_archive_backend src/archivebackend.h
    src/archivebackend.cpp
    src/archivebackend_codecs.cpp
    src/archivebackend_compression.cpp
    src/archivebackend_hash.cpp
    src/archivebackend_sourceextract.cpp
    src/archivebackend_sourceupdate.cpp
    src/archivebackend_validation.cpp
    src/archivebackend_workerprogress.cpp src/archivebackend_transfer.cpp)
target_link_libraries(sevenzip_archive_backend PRIVATE sevenzip_archive_transfer_backup)
target_sources(sevenzip_archive_backend PRIVATE src/archivecodecpublication.h)

sevenzip_add_port_component(sevenzip_compress_presentation src/createarchivedialog.h
    src/createarchivedialog.cpp
    src/createarchivedialog_advanced.cpp
    src/createarchivedialog_compression.cpp
    src/createarchivedialog_encryption.cpp
    src/createarchivedialog_formats.cpp
    src/createarchivedialog_language.cpp
    src/createarchivedialog_preferences.cpp
    src/createarchivedialog_resources.cpp
    src/createarchivedialog_sfx.cpp
    src/createarchivedialog_source.cpp
    src/createarchivedialog_volumes.cpp)

sevenzip_add_port_component(sevenzip_dialog_screen src/qtdialogscreenservices.h
    src/qtdialogscreenservices.cpp)

if(APPLE OR WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(sevenzip_window_mount_sources src/qtfilemanagermountcommands.cpp)
endif()
sevenzip_add_port_component(sevenzip_main_window src/mainwindow.h
    src/mainwindow.cpp
    src/mainwindow_archive_transfer.cpp
    src/mainwindow_archivecommands.cpp
    src/mainwindow_guioperations.cpp
    src/mainwindow_archivelist.cpp
    src/mainwindow_archiveopening.cpp
    src/mainwindow_backendevents.cpp
    src/mainwindow_close.cpp
    src/mainwindow_startup.cpp src/qtfilemanagersplitter.cpp
    src/mainwindow_columns.cpp
    src/mainwindow_refresh.cpp
    src/mainwindow_dialogs.cpp
    src/mainwindow_filelist.cpp
    src/mainwindow_fileoperations.cpp
    src/mainwindow_rename.cpp
    src/mainwindow_language.cpp
    src/mainwindow_menus.cpp
    src/mainwindow_panel.cpp
    src/mainwindow_panelkey.cpp
    src/mainwindow_shortcuts.cpp
    src/mainwindow_preferences.cpp
    src/mainwindow_propertyinvalidation.cpp
    src/mainwindow_root.cpp
    src/mainwindow_selection.cpp
    src/mainwindow_selectionpaths.cpp
    src/mainwindow_twopanel.cpp
    src/mainwindow_volumes.cpp
    src/CPP/7zip/UI/FileManager/PanelKey.cpp
    src/filepanelmodel.cpp src/filepanelmodel.h src/filetable.cpp src/filetable.h
    src/qtapppresentation.cpp
    src/qtfilemanagercommands.cpp
    src/qtfilemanagercommandworkflows.cpp
    src/qtfilemanagershellmenu.cpp
    src/qtnativeshellmenu.cpp
    src/qtprogramfilemenu.cpp
    src/qtfilemanagerdrag.cpp
    src/qtfilemanagerdragpresentation.cpp
    src/qtfilemanagerdraginput.cpp
    src/qtfilemenuprojection.cpp
    ${sevenzip_window_mount_sources})
if(APPLE OR WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(sevenzip_main_window PRIVATE sevenzip_mount_ui)
endif()

sevenzip_add_port_component(sevenzip_native_menu_presentation src/qtnativeshellmenupresentation.h
    src/qtnativeshellmenupresentation.cpp)

sevenzip_add_port_component(sevenzip_gui_entry src/qtguicommand.h src/qtguimain.cpp)
sevenzip_add_port_component(sevenzip_file_manager_entry src/qtfilemanagerstartup.h
    src/qtfilemanagermain.cpp)
if(APPLE)
    target_sources(sevenzip_file_manager_entry PRIVATE
        src/qtapplicationstartup.h src/qtapplicationstartup_mac.mm)
endif()

sevenzip_add_port_component(sevenzip_compress_source src/CPP/7zip/UI/GUI/CompressDialog.h
    src/CPP/7zip/UI/GUI/CompressDialog.cpp)

sevenzip_add_port_component(sevenzip_agent src/CPP/7zip/UI/Agent/Agent.h
    src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/AgentProxy.cpp)
sevenzip_add_port_component(sevenzip_panel_selection src/CPP/7zip/UI/FileManager/PanelSelect.h
    src/CPP/7zip/UI/FileManager/PanelSelect.cpp)
sevenzip_add_port_component(sevenzip_split_dialog src/CPP/7zip/UI/FileManager/SplitDialog.h
    src/CPP/7zip/UI/FileManager/SplitDialog.cpp)
sevenzip_add_port_component(sevenzip_panel_split src/CPP/7zip/UI/FileManager/PanelSplitFile.h
    src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/qtvolumeprogress.h src/qtvolumeprogress.cpp
    src/qtvolumeio.h src/qtvolumeio.cpp src/qtvolumeservices.h)
sevenzip_add_port_component(sevenzip_file_transfer src/filetransferoperation.h
    src/filetransferoperation.cpp src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h)
sevenzip_add_port_component(sevenzip_filesystem_scan src/filesystemscanner.h
    src/filesystemscanner.cpp)
sevenzip_add_port_component(sevenzip_volume_operations src/qtvolumetransaction.h
    src/qtvolumetransaction.cpp src/volumeoperations.h
    src/volumeoperations.cpp)

# Archive resources in this library call the typed backend even when a consumer
# only opens an options page. Carry that implementation in the resource target
# instead of requiring each executable to repeat the backend source closure.
target_link_libraries(sevenzip_location PRIVATE
    sevenzip_archive_backend sevenzip_compress_source sevenzip_agent)
target_sources(sevenzip_location PRIVATE
    src/archivebackend_guioperations.cpp
    src/archivebackend_nativeentries.cpp src/archivebackend_nativeentries.h
    src/archivebackend_test.cpp)

sevenzip_add_port_component(sevenzip_panel_context_menu src/qtpanelcontextmenu.h
    src/qtpanelcontextmenu.cpp)
set_target_properties(sevenzip_panel_context_menu PROPERTIES AUTOMOC OFF)

# Dialog-only processes do not need the File Manager window composition.
add_library(sevenzip_compression_ui STATIC)
set_target_properties(sevenzip_compression_ui PROPERTIES AUTOMOC OFF AUTOUIC OFF)
target_compile_features(sevenzip_compression_ui PUBLIC cxx_std_23)
target_link_libraries(sevenzip_compression_ui
    PUBLIC Qt6::Widgets sevenzip_location
    PRIVATE sevenzip_compress_presentation)
target_link_libraries(sevenzip_gui_entry PRIVATE sevenzip_compression_ui)
target_link_libraries(sevenzip_file_manager_entry PRIVATE sevenzip_window)
