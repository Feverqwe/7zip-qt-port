# Reuse the same production objects after all app/test consumers are registered.
# Consumers keep their existing link closure, fixtures and platform settings.
get_property(sevenzip_port_consumers DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)

# Register the narrow worker adapters for every existing module consumer before
# consolidating those sources into shared production objects.
foreach(consumer IN LISTS sevenzip_port_consumers)
    get_target_property(consumer_sources ${consumer} SOURCES)
    if("src/mainwindow.cpp" IN_LIST consumer_sources)
        target_sources(${consumer} PRIVATE src/mainwindow_guioperations.cpp)
    endif()
    if("src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp" IN_LIST consumer_sources)
        target_sources(${consumer} PRIVATE src/qtvolumeprogress.cpp src/qtvolumeprogress.h
            src/qtvolumeio.cpp src/qtvolumeio.h src/qtvolumeservices.h)
    endif()
    if("src/volumeoperations.cpp" IN_LIST consumer_sources)
        target_sources(${consumer} PRIVATE src/qtvolumetransaction.cpp src/qtvolumetransaction.h)
    endif()
endforeach()

function(sevenzip_share_port_objects owner header)
    set(module_sources ${ARGN})
    add_library(${owner} OBJECT ${module_sources} ${header})
    target_include_directories(${owner} PRIVATE src src/CPP third_party/7zip/CPP)
    target_link_libraries(${owner} PRIVATE Qt6::Widgets sevenzip_common)
    set_target_properties(${owner} PROPERTIES AUTOUIC OFF)

    foreach(consumer IN LISTS sevenzip_port_consumers)
        get_target_property(consumer_sources ${consumer} SOURCES)
        if(NOT consumer_sources)
            continue()
        endif()
        set(uses_module FALSE)
        set(missing_sources)
        foreach(source IN LISTS module_sources)
            if(source IN_LIST consumer_sources)
                set(uses_module TRUE)
            else()
                list(APPEND missing_sources ${source})
            endif()
        endforeach()
        if(NOT uses_module)
            continue()
        endif()
        if(missing_sources)
            message(FATAL_ERROR "${consumer} has an incomplete ${owner} module: ${missing_sources}")
        endif()
        list(REMOVE_ITEM consumer_sources ${module_sources} ${header})
        set_property(TARGET ${consumer} PROPERTY SOURCES ${consumer_sources})
        target_sources(${consumer} PRIVATE $<TARGET_OBJECTS:${owner}>)
    endforeach()
endfunction()

sevenzip_share_port_objects(sevenzip_archive_backend src/archivebackend.h
    src/archivebackend.cpp
    src/archivebackend_codecs.cpp
    src/archivebackend_compression.cpp
    src/archivebackend_hash.cpp
    src/archivebackend_sourceextract.cpp
    src/archivebackend_sourceupdate.cpp
    src/archivebackend_validation.cpp
    src/archivebackend_workerprogress.cpp src/archivebackend_transfer.cpp
    src/qtarchivetransferbackup.h)

sevenzip_share_port_objects(sevenzip_compress_presentation src/createarchivedialog.h
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

sevenzip_share_port_objects(sevenzip_dialog_screen src/qtdialogscreenservices.h
    src/qtdialogscreenservices.cpp)

sevenzip_share_port_objects(sevenzip_main_window src/mainwindow.h
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
    src/qtfilemenuprojection.cpp)

sevenzip_share_port_objects(sevenzip_native_menu_presentation src/qtnativeshellmenupresentation.h
    src/qtnativeshellmenupresentation.cpp)

sevenzip_share_port_objects(sevenzip_gui_entry src/qtguicommand.h src/qtguimain.cpp)
sevenzip_share_port_objects(sevenzip_file_manager_entry src/qtfilemanagerstartup.h
    src/qtfilemanagermain.cpp)
if(APPLE)
    target_sources(sevenzip_file_manager_entry PRIVATE
        src/qtapplicationstartup.h src/qtapplicationstartup_mac.mm)
endif()

sevenzip_share_port_objects(sevenzip_compress_source src/CPP/7zip/UI/GUI/CompressDialog.h
    src/CPP/7zip/UI/GUI/CompressDialog.cpp)

sevenzip_share_port_objects(sevenzip_agent src/CPP/7zip/UI/Agent/Agent.h
    src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/AgentProxy.cpp)
sevenzip_share_port_objects(sevenzip_panel_selection src/CPP/7zip/UI/FileManager/PanelSelect.h
    src/CPP/7zip/UI/FileManager/PanelSelect.cpp)
sevenzip_share_port_objects(sevenzip_split_dialog src/CPP/7zip/UI/FileManager/SplitDialog.h
    src/CPP/7zip/UI/FileManager/SplitDialog.cpp)
sevenzip_share_port_objects(sevenzip_panel_split src/CPP/7zip/UI/FileManager/PanelSplitFile.h
    src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/qtvolumeprogress.h src/qtvolumeprogress.cpp
    src/qtvolumeio.h src/qtvolumeio.cpp src/qtvolumeservices.h)
sevenzip_share_port_objects(sevenzip_file_transfer src/filetransferoperation.h
    src/filetransferoperation.cpp src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h)
sevenzip_share_port_objects(sevenzip_filesystem_scan src/filesystemscanner.h
    src/filesystemscanner.cpp)
sevenzip_share_port_objects(sevenzip_volume_operations src/qtvolumetransaction.h
    src/qtvolumetransaction.cpp src/volumeoperations.h
    src/volumeoperations.cpp)
