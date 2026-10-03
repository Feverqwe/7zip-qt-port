cmake_minimum_required(VERSION 3.21)

project(sevenzip_desktop VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Threads REQUIRED)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR (APPLE AND BUILD_TESTING))
    find_package(Qt6 REQUIRED COMPONENTS DBus)
    add_library(sevenzip_launcher STATIC src/qtlauncherprogress.cpp src/qtlauncherprogress.h)
    target_include_directories(sevenzip_launcher PUBLIC src third_party/7zip/CPP)
    target_link_libraries(sevenzip_launcher PUBLIC Qt6::DBus)
    set_target_properties(sevenzip_launcher PROPERTIES AUTOMOC ON AUTOUIC OFF)
    add_library(sevenzip_linux_taskbar STATIC src/qttaskbarprogress_linux.cpp src/qttaskbarprogress.h)
    target_link_libraries(sevenzip_linux_taskbar PUBLIC sevenzip_launcher Qt6::Widgets)
    set_target_properties(sevenzip_linux_taskbar PROPERTIES AUTOMOC OFF AUTOUIC OFF)
endif()

set(property_name_resource "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/FileManager/PropertyName.rc")
set(extract_resource "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/GUI/Extract.rc")
set(file_manager_resource "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/FileManager/resource.rc")
set(progress_resource "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/FileManager/ProgressDialog2.rc")
set(update_resource "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/GUI/resource3.rc")
set(overwrite_resource "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/FileManager/OverwriteDialog.rc")
set(gui_resource "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/FileManager/resourceGui.rc")
set(explorer_resource "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/Explorer/resource2.rc")
set(language_resource "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/FileManager/LangPage.rc")
set(resource_strings_header "${CMAKE_CURRENT_BINARY_DIR}/ResourceStrings.h")
add_custom_command(OUTPUT "${resource_strings_header}"
    COMMAND "${CMAKE_COMMAND}" "-DRESOURCE_FILES=${property_name_resource};${extract_resource};${file_manager_resource};${progress_resource};${update_resource};${overwrite_resource};${gui_resource};${explorer_resource};${language_resource}"
        "-DMIXED_RESOURCE_FILES=${file_manager_resource};${progress_resource};${overwrite_resource};${explorer_resource};${language_resource}"
        "-DOUTPUT_FILE=${resource_strings_header}" -P "${CMAKE_CURRENT_SOURCE_DIR}/scripts/rc_strings.cmake"
    DEPENDS "${property_name_resource}" "${extract_resource}" "${file_manager_resource}"
        "${progress_resource}" "${update_resource}" "${overwrite_resource}" "${gui_resource}" "${explorer_resource}" "${language_resource}" scripts/rc_strings.cmake
    VERBATIM)

qt_standard_project_setup()

if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 17)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
endif()

add_library(sevenzip_hash_registry OBJECT third_party/7zip/CPP/Common/CrcReg.cpp
    src/CPP/7zip/Archive/Rar/Rar5Handler.cpp
    third_party/7zip/CPP/Common/CRC.cpp
    third_party/7zip/CPP/Common/CksumReg.cpp third_party/7zip/CPP/Common/Md5Reg.cpp
    third_party/7zip/CPP/Common/Sha1Reg.cpp third_party/7zip/CPP/Common/Sha1Prepare.cpp
    third_party/7zip/CPP/Common/Sha256Reg.cpp third_party/7zip/CPP/Common/Sha256Prepare.cpp
    third_party/7zip/CPP/Common/Sha512Reg.cpp third_party/7zip/CPP/Common/Sha512Prepare.cpp
    third_party/7zip/CPP/Common/Sha3Reg.cpp
    third_party/7zip/CPP/Common/XzCrc64Reg.cpp third_party/7zip/CPP/Common/XzCrc64Init.cpp
    third_party/7zip/CPP/Common/Xxh64Reg.cpp)
set_target_properties(sevenzip_hash_registry PROPERTIES AUTOMOC OFF AUTOUIC OFF)
target_include_directories(sevenzip_hash_registry PRIVATE src/CPP src third_party/7zip/CPP)
target_link_libraries(sevenzip_hash_registry PRIVATE Qt6::Core)
add_library(sevenzip_hash_registration INTERFACE)
target_link_libraries(sevenzip_hash_registration INTERFACE sevenzip_hash_registry
    $<TARGET_OBJECTS:sevenzip_hash_registry>)

add_library(sevenzip_common STATIC third_party/7zip/C/Sort.c third_party/7zip/C/Alloc.c
    third_party/7zip/C/7zCrc.c third_party/7zip/C/7zCrcOpt.c third_party/7zip/C/CpuArch.c
    third_party/7zip/C/Sha1.c third_party/7zip/C/Sha1Opt.c
    third_party/7zip/C/Sha256.c third_party/7zip/C/Sha256Opt.c
    third_party/7zip/C/Sha512.c third_party/7zip/C/Sha512Opt.c third_party/7zip/C/Sha3.c
    third_party/7zip/C/Md5.c third_party/7zip/C/XzCrc64.c third_party/7zip/C/XzCrc64Opt.c
    third_party/7zip/C/Xxh64.c third_party/7zip/CPP/7zip/Compress/BZip2Crc.cpp
    third_party/7zip/C/Blake2s.c
    third_party/7zip/CPP/7zip/Common/CreateCoder.cpp
    third_party/7zip/CPP/7zip/Common/FilterCoder.cpp
    src/CPP/7zip/UI/Common/HashCalc.cpp
    src/CPP/7zip/UI/Common/Bench.cpp src/CPP/7zip/UI/Common/Bench.h
    src/qtbenchmarkservices.cpp src/qtbenchmarkservices.h
    third_party/7zip/CPP/Windows/SystemInfo.cpp
    third_party/7zip/CPP/Common/DynLimBuf.cpp
    src/CPP/7zip/UI/Common/EnumDirItems.cpp src/CPP/7zip/UI/Common/EnumDirItems.h
    src/CPP/7zip/UI/Common/SortUtils.cpp
    src/CPP/Windows/FileLink.cpp
    third_party/7zip/CPP/7zip/Common/UniqBlocks.cpp
    third_party/7zip/CPP/7zip/Common/ProgressUtils.cpp
    src/CPP/7zip/UI/Common/LoadCodecs.cpp src/CPP/7zip/UI/Common/LoadCodecs.h
    third_party/7zip/CPP/7zip/UI/Common/DefaultName.cpp
    src/processpriority.cpp src/processpriority.h
    src/processpause.cpp src/processpause.h
    third_party/7zip/C/Threads.c third_party/7zip/CPP/Windows/ErrorMsg.cpp
    src/CPP/Windows/DLL.cpp src/CPP/Windows/DLL.h
    third_party/7zip/CPP/Common/StringToInt.cpp
    third_party/7zip/CPP/Common/MyString.cpp third_party/7zip/CPP/Common/IntToString.cpp
    third_party/7zip/CPP/Common/MyWindows.cpp
    src/qtownedtempidentity.cpp src/qtownedtempidentity.h
    src/qtownedtempregistry.cpp src/qtownedtempregistry.h
    src/CPP/Windows/FileDir.cpp src/CPP/Windows/FileDir.h
    src/CPP/Windows/FileName.cpp
    third_party/7zip/CPP/Windows/FileFind.cpp
    third_party/7zip/CPP/Windows/FileIO.cpp
    third_party/7zip/CPP/Windows/TimeUtils.cpp
    third_party/7zip/CPP/7zip/Common/FileStreams.cpp
    third_party/7zip/CPP/7zip/Common/StreamUtils.cpp
    third_party/7zip/CPP/7zip/Common/StreamObjects.cpp
    third_party/7zip/CPP/7zip/Common/LimitedStreams.cpp
    third_party/7zip/CPP/7zip/Compress/CopyCoder.cpp
    third_party/7zip/CPP/7zip/Archive/Common/ItemNameUtils.cpp
    src/CPP/7zip/UI/Common/ExtractingFilePath.cpp src/CPP/7zip/UI/Common/ExtractingFilePath.h
    src/CPP/Common/Wildcard.cpp src/CPP/Common/Wildcard.h
    third_party/7zip/CPP/Common/StringConvert.cpp third_party/7zip/CPP/Common/UTFConvert.cpp
    third_party/7zip/CPP/Windows/System.cpp src/CPP/7zip/Common/MethodProps.cpp src/CPP/7zip/UI/Common/ZipRegistry.cpp
    third_party/7zip/CPP/Windows/PropVariant.cpp third_party/7zip/CPP/Windows/PropVariantConv.cpp
    src/CPP/7zip/UI/FileManager/ClassDefs.cpp
    third_party/7zip/CPP/7zip/Common/PropId.cpp
    src/CPP/7zip/UI/FileManager/ViewSettings.cpp src/CPP/7zip/UI/FileManager/ViewSettings.h
    src/CPP/7zip/UI/FileManager/RegistryUtils.cpp src/CPP/7zip/UI/FileManager/RegistryUtils.h)
set_target_properties(sevenzip_common PROPERTIES AUTOMOC OFF AUTOUIC OFF)
target_include_directories(sevenzip_common PUBLIC src/CPP third_party/7zip/CPP PRIVATE src)
target_include_directories(sevenzip_common PRIVATE third_party/7zip/CPP/Windows)
target_link_libraries(sevenzip_common PUBLIC Qt6::Core)
target_compile_definitions(sevenzip_common PUBLIC Z7_EXTERNAL_CODECS)
target_compile_definitions(sevenzip_hash_registry PRIVATE Z7_EXTERNAL_CODECS)
target_link_libraries(sevenzip_common PUBLIC sevenzip_hash_registration)
target_link_libraries(sevenzip_common PUBLIC ${CMAKE_DL_LIBS})
target_link_libraries(sevenzip_common PUBLIC Threads::Threads)
if(WIN32)
    target_sources(sevenzip_common PRIVATE third_party/7zip/CPP/Windows/Registry.cpp)
    target_sources(sevenzip_common PRIVATE third_party/7zip/CPP/Windows/ProcessUtils.cpp)
    set_source_files_properties(third_party/7zip/CPP/Windows/ProcessUtils.cpp
        third_party/7zip/CPP/Windows/Registry.cpp src/CPP/7zip/UI/Common/LoadCodecs.cpp PROPERTIES
        COMPILE_DEFINITIONS "_UNICODE;UNICODE")
    target_sources(sevenzip_common PRIVATE third_party/7zip/CPP/Windows/SecurityUtils.cpp)
    target_link_libraries(sevenzip_common PUBLIC ole32 oleaut32 uuid advapi32)
endif()

include(cmake/native_zip.cmake)
include(cmake/native_rar.cmake)

add_library(sevenzip_panel STATIC src/CPP/7zip/UI/FileManager/App.cpp
    src/CPP/7zip/UI/FileManager/PanelCrc.cpp
    src/CPP/7zip/UI/Common/ArchiveName.cpp
    src/qtpaneldragmodel.h
    src/qtpaneldragtransport.cpp src/qtpaneldragtransport.h
    src/CPP/7zip/UI/FileManager/AboutDialog.cpp src/CPP/7zip/UI/FileManager/AboutDialog.h
    src/CPP/7zip/UI/FileManager/BrowseDialog2.cpp src/CPP/7zip/UI/FileManager/BrowseDialog2.h
    src/qttempbrowsepresentation.cpp
    src/qtfilelisthover.cpp src/qtfilelisthover.h
    src/CPP/7zip/UI/FileManager/HelpUtils.cpp src/CPP/7zip/UI/FileManager/HelpUtils.h
    src/qthelpservices.cpp src/qthelpservices.h
    src/qtcompressoptionscontrols.cpp src/qtcompressoptionscontrols.h
    src/qtcompressoptionsdialog.h
    src/CPP/7zip/UI/GUI/UpdateGUI.cpp src/CPP/7zip/UI/GUI/UpdateGUI.h
    third_party/7zip/CPP/7zip/UI/Common/UpdateAction.cpp
    src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h
    src/CPP/7zip/UI/FileManager/CopyDialog.cpp src/CPP/7zip/UI/FileManager/CopyDialog.h
    src/CPP/Windows/Control/Dialog.cpp src/CPP/Windows/Control/Dialog.h
    src/CPP/7zip/UI/FileManager/MyLoadMenu.cpp src/CPP/7zip/UI/FileManager/MyLoadMenu.h
    src/qtfilemenuservices.h src/qtfilemanagercommandservices.h
    src/CPP/7zip/UI/FileManager/FM.cpp src/qtfilemanagerwindowservices.h
    src/qtappoperationstate.cpp src/qtappoperationstate.h
    src/CPP/7zip/UI/FileManager/PanelDrag.cpp
    src/CPP/7zip/UI/GUI/HashGUI.cpp src/CPP/7zip/UI/GUI/HashGUI.h
    src/CPP/7zip/UI/GUI/BenchmarkDialog.cpp src/CPP/7zip/UI/GUI/BenchmarkDialog.h
    src/CPP/7zip/UI/GUI/CompressDialog.cpp
    src/CPP/7zip/UI/Common/CompressCall2.cpp src/CPP/7zip/UI/Common/CompressCall.h
    src/qtbenchmarkcontrols.cpp src/qtbenchmarkcontrols.h
    src/CPP/7zip/UI/FileManager/ExtractCallback.cpp src/CPP/7zip/UI/FileManager/ExtractCallback.h
    src/qtcallbackdialogs.cpp src/qtcallbackdialogs.h src/qtmodalresult.h
    src/CPP/7zip/UI/FileManager/OverwriteDialog.cpp src/CPP/7zip/UI/FileManager/OverwriteDialog.h
    src/CPP/7zip/UI/FileManager/MemDialog.cpp src/CPP/7zip/UI/FileManager/MemDialog.h
    src/CPP/7zip/UI/FileManager/OpenCallback.cpp src/CPP/7zip/UI/FileManager/OpenCallback.h
    src/CPP/7zip/UI/FileManager/FileFolderPluginOpen.cpp src/CPP/7zip/UI/FileManager/FileFolderPluginOpen.h
    src/CPP/7zip/UI/FileManager/PanelItemOpen.cpp
    src/CPP/7zip/UI/FileManager/VerCtrl.cpp
    src/qtverctrlfileservices.cpp src/qtverctrlfileservices.h
    src/qtassociatedfileservices.cpp src/qtassociatedfileservices.h
    src/qtexternalprocess.cpp src/qtexternalprocess.h
    src/qtexternalguicall.cpp src/qtexternalguicall.h
    src/CPP/7zip/UI/FileManager/PanelCopy.cpp
    src/CPP/7zip/UI/Explorer/ContextMenu.cpp
    src/CPP/7zip/UI/FileManager/RootFolder.cpp src/CPP/7zip/UI/FileManager/RootFolder.h
    src/archiveprogressoperation.cpp src/archiveprogressoperation.h
    src/archiveprogresstransport.cpp src/archiveprogresstransport.h src/qtprogresstransport.h
    src/qtcallbackprogresstransport.cpp src/qtcallbackprogresstransport.h
    src/CPP/7zip/UI/FileManager/FormatUtils.cpp src/CPP/7zip/UI/FileManager/FormatUtils.h
    src/CPP/7zip/UI/FileManager/ProgressDialog2.cpp src/CPP/7zip/UI/FileManager/ProgressDialog2.h
    src/CPP/7zip/UI/FileManager/MessagesDialog.cpp src/CPP/7zip/UI/FileManager/MessagesDialog.h
    src/CPP/7zip/UI/FileManager/MessagesDialogRes.h
    src/CPP/7zip/UI/GUI/UpdateCallbackGUI2.cpp src/CPP/7zip/UI/GUI/UpdateCallbackGUI2.h
    src/CPP/7zip/UI/GUI/ExtractGUI.cpp src/CPP/7zip/UI/GUI/ExtractGUI.h
    src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h
    src/CPP/7zip/UI/FileManager/UpdateCallback100.cpp src/CPP/7zip/UI/FileManager/UpdateCallback100.h
    src/qtcallbackpassword.cpp src/qtcallbackpassword.h
    src/qtprogresscontrols.cpp src/qtprogresscontrols.h
    src/qtpanellist.cpp src/qtpanellist.h
    src/qtpanelfolder.cpp src/qtpanelfolderraw.cpp src/qtpanelfolder.h
    src/CPP/7zip/UI/FileManager/App.h src/CPP/7zip/UI/FileManager/Panel.cpp
    src/CPP/7zip/UI/FileManager/SettingsPage.cpp src/CPP/7zip/UI/FileManager/SettingsPage.h
    src/qtmemorylimitspin.cpp src/qtmemorylimitspin.h
    src/CPP/7zip/UI/FileManager/FoldersPage.cpp src/CPP/7zip/UI/FileManager/FoldersPage.h
    src/CPP/7zip/UI/FileManager/PanelFolderChange.cpp
    src/CPP/7zip/UI/FileManager/PanelMenu.cpp
    src/CPP/7zip/UI/FileManager/SysIconUtils.cpp src/CPP/7zip/UI/FileManager/SysIconUtils.h
    src/CPP/7zip/UI/FileManager/PropertyName.cpp src/CPP/7zip/UI/FileManager/PropertyName.h
    src/qtlangcontrols.cpp src/qtlangcontrols.h
    src/qtstatuslabel.cpp src/qtstatuslabel.h
    src/CPP/7zip/UI/FileManager/PanelItems.cpp
    src/CPP/7zip/UI/FileManager/PanelOperations.cpp
    src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h
    src/qtlistview.cpp src/qtlistview.h
    src/CPP/7zip/UI/FileManager/AppState.h
    src/CPP/7zip/UI/FileManager/ListViewDialog.cpp src/CPP/7zip/UI/FileManager/ListViewDialog.h
    src/CPP/7zip/UI/FileManager/EditDialog.cpp src/CPP/7zip/UI/FileManager/EditDialog.h
    src/CPP/7zip/UI/FileManager/ComboDialog.cpp src/CPP/7zip/UI/FileManager/ComboDialog.h
    src/CPP/7zip/UI/FileManager/PasswordDialog.cpp src/CPP/7zip/UI/FileManager/PasswordDialog.h
    src/CPP/7zip/UI/FileManager/BrowseDialog.cpp src/CPP/7zip/UI/FileManager/BrowseDialog.h
    src/CPP/7zip/UI/FileManager/NetFolder.cpp src/CPP/7zip/UI/FileManager/NetFolder.h
    src/CPP/7zip/UI/FileManager/PanelListNotify.cpp src/CPP/7zip/UI/FileManager/Panel.h)
set_target_properties(sevenzip_panel PROPERTIES AUTOUIC OFF)
qt_add_resources(sevenzip_panel upstream_progress_icons
    PREFIX "/upstream/7zip/UI/FileManager"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/FileManager"
    FILES third_party/7zip/CPP/7zip/UI/FileManager/FM.ico
          third_party/7zip/CPP/7zip/UI/FileManager/Add.bmp
          third_party/7zip/CPP/7zip/UI/FileManager/Add2.bmp
          third_party/7zip/CPP/7zip/UI/FileManager/Extract.bmp
          third_party/7zip/CPP/7zip/UI/FileManager/Extract2.bmp
          third_party/7zip/CPP/7zip/UI/FileManager/Test.bmp
          third_party/7zip/CPP/7zip/UI/FileManager/Test2.bmp
          third_party/7zip/CPP/7zip/UI/FileManager/Copy.bmp
          third_party/7zip/CPP/7zip/UI/FileManager/Copy2.bmp
          third_party/7zip/CPP/7zip/UI/FileManager/Move.bmp
          third_party/7zip/CPP/7zip/UI/FileManager/Move2.bmp
          third_party/7zip/CPP/7zip/UI/FileManager/Delete.bmp
          third_party/7zip/CPP/7zip/UI/FileManager/Delete2.bmp
          third_party/7zip/CPP/7zip/UI/FileManager/Info.bmp
          third_party/7zip/CPP/7zip/UI/FileManager/Info2.bmp)
target_include_directories(sevenzip_panel PUBLIC src)
target_include_directories(sevenzip_panel PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(sevenzip_panel PUBLIC Qt6::Widgets sevenzip_common)
if(WIN32)
    target_link_libraries(sevenzip_panel PUBLIC shell32)
endif()
target_sources(sevenzip_panel PRIVATE src/qtprogresswait.cpp src/qtprogresswait.h)
if(APPLE)
    target_sources(sevenzip_panel PRIVATE src/qttaskbarprogress_mac.mm src/qttaskbarprogress.h)
    target_link_libraries(sevenzip_panel PUBLIC "-framework AppKit")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(sevenzip_panel PUBLIC sevenzip_linux_taskbar)
endif()

add_library(sevenzip_filesystem STATIC
    src/archivesfxmodule.cpp src/archivesfxmodule.h
    src/qtarchivehashoperation.cpp src/qtarchivehashoperation.h
    src/archiveenginelocation.cpp src/archiveenginelocation.h
    src/qtownedtempdir.cpp src/qtownedtempdir.h
    third_party/7zip/CPP/Common/Lang.cpp
    src/CPP/7zip/UI/FileManager/LangUtils.cpp src/CPP/7zip/UI/FileManager/LangUtils.h
    src/qtlanguageservices.cpp src/qtlanguageservices.h
    src/CPP/Windows/ResourceString.cpp src/CPP/Windows/ResourceString.h
    "${resource_strings_header}"
    src/CPP/7zip/UI/Common/WorkDir.cpp src/CPP/7zip/UI/Common/WorkDir.h
    src/CPP/7zip/UI/Common/Update.cpp src/CPP/7zip/UI/Common/Update.h
    src/qtarchivedeleteinputs.cpp src/qtarchivedeleteinputs.h
    src/qtarchivecreationlifecycle.cpp src/qtarchivecreationlifecycle.h
    src/CPP/7zip/Common/MultiOutStream.cpp
    src/qtarchivevolumeoutput.cpp src/qtarchivevolumeoutput.h
    src/CPP/7zip/UI/Common/ArchiveExtractCallback.cpp src/CPP/7zip/UI/Common/ArchiveExtractCallback.h
    src/CPP/7zip/UI/Common/Extract.cpp src/CPP/7zip/UI/Common/Extract.h
    src/CPP/7zip/UI/Common/OpenArchive.cpp src/CPP/7zip/UI/Common/OpenArchive.h
    src/archivecodeccatalog.cpp src/archivecodeccatalog.h
    src/archivecodecimage.cpp src/archivecodecimage.h
    src/archivenativeopen.cpp src/archivenativeopen.h
    src/archivecodecprobe.cpp src/archivecodecprobe.h
    third_party/7zip/CPP/7zip/UI/Common/ArchiveOpenCallback.cpp
    third_party/7zip/CPP/7zip/UI/Common/SetProperties.cpp
    third_party/7zip/CPP/7zip/Common/FilePathAutoRename.cpp
    src/CPP/7zip/UI/FileManager/FSFolder.cpp
    src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp
    src/qtfiletransfercopycallback.cpp src/qtfiletransfercopycallback.h
    src/CPP/7zip/UI/FileManager/TextPairs.cpp
    src/CPP/7zip/UI/FileManager/FSDrives.cpp src/CPP/7zip/UI/FileManager/FSDrives.h
    src/qtvolumemounts.cpp src/qtvolumemounts.h
    third_party/7zip/CPP/7zip/UI/Common/PropIDUtils.cpp)
qt_add_resources(sevenzip_filesystem upstream_languages PREFIX "/upstream/7zip/Lang"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources/7zip/Lang"
    FILES resources/7zip/Lang/en.ttt resources/7zip/Lang/ru.txt)
file(GLOB_RECURSE sevenzip_help_assets CONFIGURE_DEPENDS resources/7zip/Help/*.htm resources/7zip/Help/*.css)
qt_add_resources(sevenzip_panel upstream_help PREFIX "/upstream/7zip/Help"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources/7zip/Help" FILES ${sevenzip_help_assets})
target_compile_definitions(sevenzip_panel PRIVATE SEVENZIP_DESKTOP_VERSION="${PROJECT_VERSION}")
qt_add_resources(sevenzip_panel upstream_about PREFIX "/upstream/7zip/About"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/FileManager"
    FILES third_party/7zip/CPP/7zip/UI/FileManager/7zipLogo.ico)
if(WIN32)
    target_sources(sevenzip_filesystem PRIVATE third_party/7zip/CPP/Windows/FileSystem.cpp
        third_party/7zip/CPP/Windows/Net.cpp third_party/7zip/CPP/Windows/Net.h)
    target_link_libraries(sevenzip_filesystem PUBLIC mpr)
endif()
target_include_directories(sevenzip_filesystem PUBLIC src)
target_include_directories(sevenzip_filesystem PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(sevenzip_filesystem PRIVATE third_party/7zip/CPP/Windows)
target_link_libraries(sevenzip_filesystem PUBLIC Qt6::Core sevenzip_common sevenzip_zip)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/docs/upstream-baseline.json" sfx_baseline_json)
string(JSON sfx_upstream_revision GET "${sfx_baseline_json}" revision)
target_compile_definitions(sevenzip_filesystem PRIVATE
    "SEVENZIP_UPSTREAM_REVISION=\"${sfx_upstream_revision}\"")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(sevenzip_filesystem PRIVATE
        "SEVENZIP_BUILD_SFX_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/build/sfx\"")
    target_compile_definitions(sevenzip_filesystem PRIVATE
        "SEVENZIP_BUILD_CODEC_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\"")
endif()
target_link_libraries(sevenzip_panel PUBLIC sevenzip_filesystem)

add_library(sevenzip_location STATIC src/qtlocationfolder.cpp src/qtlocationfolder.h
    src/CPP/7zip/UI/FileManager/LangPage.cpp src/CPP/7zip/UI/FileManager/LangPage.h
    src/CPP/7zip/UI/FileManager/EditPage.cpp src/CPP/7zip/UI/FileManager/EditPage.h
    src/CPP/7zip/UI/FileManager/MenuPage.cpp src/CPP/7zip/UI/FileManager/MenuPage.h
    src/CPP/7zip/UI/FileManager/OptionsDialog.cpp src/qtfmoptionsdialog.h
    src/qtarchivebatchopen.cpp src/qtarchivebatchopen.h
    src/qtarchivebatchextract.cpp src/qtarchivebatchextract.h
    src/qtarchivebackendrequest.cpp src/qtarchivebackendrequest.h
    src/qtarchivesourceupdate.cpp src/qtarchivesourceupdate.h
    src/qtarchivereopen.cpp src/qtarchivereopen.h
    src/qtarchivesourceextract.cpp
    src/CPP/7zip/UI/Agent/AgentOut.cpp
    src/CPP/7zip/UI/Agent/ArchiveFolderOut.cpp
    src/CPP/7zip/UI/Agent/ArchiveFolder.cpp
    src/CPP/7zip/UI/Common/UpdateCallback.cpp src/CPP/7zip/UI/Common/UpdateCallback.h
    src/CPP/7zip/UI/Agent/UpdateCallbackAgent.cpp src/CPP/7zip/UI/Agent/UpdateCallbackAgent.h
    src/CPP/7zip/UI/Common/UpdatePair.cpp
    third_party/7zip/CPP/7zip/UI/Common/UpdateProduce.cpp
    src/qtarchiveproperties.cpp src/qtarchiveproperties.h
    src/qtarchivefoldermanager.cpp src/qtarchivefoldermanager.h
    src/qtarchivenativeopenprogress.cpp src/qtarchivenativeopenprogress.h
    src/qtarchivestreamopen.h
    src/qtarchivefileresource.cpp src/qtarchivefileresource.h
    src/qtarchivenativeproperties.cpp
    src/qtfolderpropertycapabilities.h
    src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/AgentProxy.cpp)
target_include_directories(sevenzip_location PUBLIC src)
target_link_libraries(sevenzip_location PUBLIC sevenzip_filesystem sevenzip_panel)

add_library(sevenzip_folder_loader STATIC
    src/filesystemfolderloader.cpp src/filesystemfolderloader.h)
target_include_directories(sevenzip_folder_loader PUBLIC src)
target_link_libraries(sevenzip_folder_loader PUBLIC sevenzip_location Qt6::Core)

qt_add_executable(sevenzip_desktop
    src/main.cpp src/desktopidentity.h
      src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp
    src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h
    src/combodialog.h
    src/copydialog.h
    src/extractdialog.h
    src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp
    src/archivebackend.h
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

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(drag_drop_tests tests/drag_drop_tests.cpp)
    target_link_libraries(drag_drop_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME drag_drop COMMAND drag_drop_tests -silent)
    set_tests_properties(drag_drop PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    qt_add_executable(owned_temp_tests tests/owned_temp_tests.cpp)
    target_include_directories(owned_temp_tests PRIVATE src)
    target_link_libraries(owned_temp_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME owned_temp COMMAND owned_temp_tests -silent)
    qt_add_executable(temp_cleanup_tests tests/temp_cleanup_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(temp_cleanup_tests PRIVATE tests)
    target_link_libraries(temp_cleanup_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME temp_cleanup COMMAND temp_cleanup_tests -silent)
    set_tests_properties(temp_cleanup PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/temp-cleanup-settings.ini" TIMEOUT 30)
    set_tests_properties(owned_temp PROPERTIES TIMEOUT 30)
    qt_add_executable(help_source_tests tests/help_source_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(help_source_tests PRIVATE src tests)
    target_link_libraries(help_source_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME help_source COMMAND help_source_tests -silent)
    set_tests_properties(help_source PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    if(TARGET sevenzip_launcher)
        find_program(DBUS_DAEMON_EXECUTABLE dbus-daemon REQUIRED)
        add_library(private_bus_fixture STATIC tests/fixtures/private_bus.cpp tests/fixtures/private_bus.h)
        target_link_libraries(private_bus_fixture PUBLIC Qt6::DBus)
        set_target_properties(private_bus_fixture PROPERTIES AUTOMOC OFF AUTOUIC OFF)
        target_compile_definitions(private_bus_fixture PRIVATE
            "DBUS_DAEMON_PATH=\"${DBUS_DAEMON_EXECUTABLE}\"")
        qt_add_executable(launcher_progress_tests tests/launcher_progress_tests.cpp)
        target_link_libraries(launcher_progress_tests PRIVATE sevenzip_launcher private_bus_fixture Qt6::Test)
        add_test(NAME launcher_progress COMMAND launcher_progress_tests -silent)
        set_tests_properties(launcher_progress PROPERTIES TIMEOUT 20)
        qt_add_executable(taskbar_adapter_tests tests/taskbar_adapter_tests.cpp src/desktopidentity.h
            tests/fixtures/taskbar_adapter_child.cpp tests/fixtures/taskbar_adapter_child.h)
        target_link_libraries(taskbar_adapter_tests PRIVATE sevenzip_linux_taskbar private_bus_fixture Qt6::Test)
        add_test(NAME taskbar_adapter COMMAND taskbar_adapter_tests -silent)
        set_tests_properties(taskbar_adapter PROPERTIES TIMEOUT 30)
    endif()
    qt_add_executable(password_dialog_tests tests/password_dialog_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(password_dialog_tests PRIVATE tests)
    target_link_libraries(password_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME password_dialog COMMAND password_dialog_tests -silent)
    set_tests_properties(password_dialog PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(edit_dialog_tests tests/edit_dialog_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(edit_dialog_tests PRIVATE tests)
    target_link_libraries(edit_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME edit_dialog COMMAND edit_dialog_tests -silent)
    set_tests_properties(edit_dialog PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(list_view_dialog_tests tests/list_view_dialog_tests.cpp)
    target_link_libraries(list_view_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME list_view_dialog COMMAND list_view_dialog_tests -silent)
    set_tests_properties(list_view_dialog PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(list_activation_tests tests/list_activation_tests.cpp)
    target_link_libraries(list_activation_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME list_activation COMMAND list_activation_tests -silent)
    set_tests_properties(list_activation PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(folder_history_tests tests/folder_history_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(folder_history_tests PRIVATE tests)
    target_link_libraries(folder_history_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME folder_history COMMAND folder_history_tests -silent)
    set_tests_properties(folder_history PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
    add_test(NAME upstream_baseline COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/upstream_baseline_tests.py)
    add_test(NAME upstream_inventory COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/upstream_baseline.py inventory)
    qt_add_executable(app_state_tests tests/app_state_tests.cpp)
    target_link_libraries(app_state_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME app_state COMMAND app_state_tests -silent)
    qt_add_executable(view_settings_columns_tests tests/view_settings_columns_tests.cpp)
    target_include_directories(view_settings_columns_tests PRIVATE src)
    target_link_libraries(view_settings_columns_tests PRIVATE sevenzip_common Qt6::Test)
    add_test(NAME view_settings_columns COMMAND view_settings_columns_tests -silent)
    qt_add_executable(folder_contract_tests tests/folder_contract_tests.cpp)
    target_link_libraries(folder_contract_tests PRIVATE sevenzip_common Qt6::Test)
    add_test(NAME folder_contract COMMAND folder_contract_tests -silent)
    qt_add_executable(panel_folder_tests tests/panel_folder_tests.cpp)
    target_link_libraries(panel_folder_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME panel_folder COMMAND panel_folder_tests -silent)
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
    foreach(prompt IN ITEMS overwrite mem)
        add_test(NAME ui_parity_${prompt} COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_ui_parity.py compare
            --reference ${CMAKE_CURRENT_SOURCE_DIR}/docs/ui-reference/${prompt}-dialog.reference.json
            --contract ${CMAKE_CURRENT_SOURCE_DIR}/docs/ui-reference/${prompt}-dialog.contract.json
            --reports ${CMAKE_BINARY_DIR}/ui-reports
            --output ${CMAKE_BINARY_DIR}/ui-reports/${prompt}-parity.json)
        set_tests_properties(ui_parity_${prompt} PROPERTIES
            FIXTURES_REQUIRED ui_reference_extract_prompts)
    endforeach()
    qt_add_executable(combo_dialog_tests tests/combo_dialog_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(combo_dialog_tests PRIVATE tests)
    target_link_libraries(combo_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME combo_dialog COMMAND combo_dialog_tests -silent)
    qt_add_executable(browse_dialog_tests tests/browse_dialog_tests.cpp)
    target_link_libraries(browse_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME browse_dialog COMMAND browse_dialog_tests -silent)
    set_tests_properties(browse_dialog PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)
    set_tests_properties(combo_dialog PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(volume_dialog_tests tests/volume_dialog_tests.cpp tests/layout_inspection.cpp
        src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitUtils.cpp
        src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp)
    target_include_directories(volume_dialog_tests PRIVATE src tests)
    target_link_libraries(volume_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME volume_dialog COMMAND volume_dialog_tests -silent)
    set_tests_properties(volume_dialog PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(filesystem_operations_tests tests/filesystem_operations_tests.cpp
        tests/filedeleteoperation.cpp tests/filedeleteoperation.h)
    target_link_libraries(filesystem_operations_tests PRIVATE sevenzip_filesystem Qt6::Test)
    add_test(NAME filesystem_operations COMMAND filesystem_operations_tests -silent)
    qt_add_executable(filesystem_cleanup_tests tests/filesystem_cleanup_tests.cpp)
    target_link_libraries(filesystem_cleanup_tests PRIVATE sevenzip_filesystem Qt6::Test)
    add_test(NAME filesystem_cleanup COMMAND filesystem_cleanup_tests -silent)
    add_library(native_library_fixture SHARED tests/fixtures/native_library_fixture.c)
    set_target_properties(native_library_fixture PROPERTIES AUTOMOC OFF AUTOUIC OFF)
    qt_add_executable(native_library_tests tests/native_library_tests.cpp)
    target_include_directories(native_library_tests PRIVATE src)
    target_link_libraries(native_library_tests PRIVATE sevenzip_common Qt6::Test)
    target_compile_definitions(native_library_tests PRIVATE
        "NATIVE_LIBRARY_FIXTURE=\"$<TARGET_FILE:native_library_fixture>\"")
    add_dependencies(native_library_tests native_library_fixture)
    add_test(NAME native_library COMMAND native_library_tests -silent)
    find_file(SEVENZIP_CODEC_MODULE NAMES 7z.so 7z.dll
        HINTS ENV SEVENZIP_CODEC_MODULE_DIR
        PATHS /opt/homebrew/opt/sevenzip/lib /usr/lib/7zip /usr/lib/p7zip
        DOC "Official 7-Zip module for native capability regression checks")
    qt_add_executable(codec_capabilities_tests tests/codec_capabilities_tests.cpp)
    target_include_directories(codec_capabilities_tests PRIVATE src)
    target_link_libraries(codec_capabilities_tests PRIVATE sevenzip_filesystem Qt6::Test)
    target_compile_definitions(codec_capabilities_tests PRIVATE
        "NATIVE_LIBRARY_FIXTURE=\"$<TARGET_FILE:native_library_fixture>\"")
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(codec_capabilities_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_dependencies(codec_capabilities_tests native_library_fixture)
    add_test(NAME codec_capabilities COMMAND codec_capabilities_tests -silent)
    qt_add_executable(folder_link_tests tests/folder_link_tests.cpp)
    target_link_libraries(folder_link_tests PRIVATE sevenzip_panel Qt6::Test)
    target_compile_definitions(folder_link_tests PRIVATE
        "NATIVE_LIBRARY_FIXTURE=\"$<TARGET_FILE:native_library_fixture>\"")
    add_dependencies(folder_link_tests native_library_fixture)
    add_test(NAME folder_link COMMAND folder_link_tests -silent)
    qt_add_executable(panel_processing_tests tests/panel_processing_tests.cpp)
    target_link_libraries(panel_processing_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME panel_processing COMMAND panel_processing_tests -silent)
    set_tests_properties(panel_processing PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(format_utils_tests tests/format_utils_tests.cpp)
    target_link_libraries(format_utils_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME format_utils COMMAND format_utils_tests -silent)
    qt_add_executable(native_services_tests tests/native_services_tests.cpp)
    target_link_libraries(native_services_tests PRIVATE sevenzip_common Qt6::Test)
    add_test(NAME native_services COMMAND native_services_tests -silent)
    set_tests_properties(native_services PROPERTIES TIMEOUT 10)
    qt_add_executable(progress_sync_tests tests/progress_sync_tests.cpp)
    target_link_libraries(progress_sync_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME progress_sync COMMAND progress_sync_tests -silent)
    qt_add_executable(progress_wait_tests tests/progress_wait_tests.cpp)
    target_link_libraries(progress_wait_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME progress_wait COMMAND progress_wait_tests -silent)
    set_tests_properties(progress_wait PROPERTIES TIMEOUT 10)
    qt_add_executable(progress_worker_tests tests/progress_worker_tests.cpp
        tests/layout_inspection.cpp)
    target_link_libraries(progress_worker_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME progress_worker COMMAND progress_worker_tests -silent)
    set_tests_properties(progress_worker PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports"
        FIXTURES_SETUP ui_reference_progress_worker TIMEOUT 30)
    qt_add_executable(update_callback_tests tests/update_callback_tests.cpp)
    qt_add_executable(open_callback_tests tests/open_callback_tests.cpp)
    qt_add_executable(work_dir_tests tests/work_dir_tests.cpp)
    qt_add_executable(source_update_backend_tests tests/source_update_backend_tests.cpp
        src/archivebackend.cpp src/archivebackend_listing.cpp src/archivebackend_mutations.cpp
        src/archivebackend_validation.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp)
    qt_add_executable(source_extract_backend_tests tests/source_extract_backend_tests.cpp
        src/archivebackend.cpp src/archivebackend_listing.cpp src/archivebackend_mutations.cpp
        src/archivebackend_validation.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp)
    target_include_directories(source_extract_backend_tests PRIVATE src)
    target_link_libraries(source_extract_backend_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME source_extract_backend COMMAND source_extract_backend_tests -silent)
    set_tests_properties(source_extract_backend PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    qt_add_executable(source_extract_contract_tests tests/source_extract_contract_tests.cpp
        src/archivebackend.cpp src/archivebackend_listing.cpp src/archivebackend_mutations.cpp
        src/archivebackend_validation.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp)
    target_include_directories(source_extract_contract_tests PRIVATE src)
    target_link_libraries(source_extract_contract_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME source_extract_contract COMMAND source_extract_contract_tests -silent)
    set_tests_properties(source_extract_contract PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    qt_add_executable(nested_update_tests tests/nested_update_tests.cpp
        src/archivebackend.cpp src/archivebackend_listing.cpp src/archivebackend_mutations.cpp
        src/archivebackend_validation.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp)
    target_include_directories(nested_update_tests PRIVATE src)
    target_link_libraries(nested_update_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME nested_update COMMAND nested_update_tests -silent)
    set_tests_properties(nested_update PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    target_include_directories(source_update_backend_tests PRIVATE src)
    target_link_libraries(source_update_backend_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME source_update_backend COMMAND source_update_backend_tests -silent)
    set_tests_properties(source_update_backend PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    get_target_property(owned_temp_publication_sources source_update_backend_tests SOURCES)
    list(REMOVE_ITEM owned_temp_publication_sources tests/source_update_backend_tests.cpp)
    qt_add_executable(drag_archive_tests tests/drag_archive_tests.cpp ${owned_temp_publication_sources})
    target_link_libraries(drag_archive_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME drag_archive COMMAND drag_archive_tests -silent)
    set_tests_properties(drag_archive PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    qt_add_executable(owned_temp_publication_tests tests/owned_temp_publication_tests.cpp
        ${owned_temp_publication_sources})
    target_include_directories(owned_temp_publication_tests PRIVATE src)
    target_link_libraries(owned_temp_publication_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME owned_temp_publication COMMAND owned_temp_publication_tests -silent)
    set_tests_properties(owned_temp_publication PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    qt_add_executable(archive_update_callback_tests tests/archive_update_callback_tests.cpp)
    qt_add_executable(native_extract_callback_tests tests/native_extract_callback_tests.cpp)
    qt_add_executable(native_hash_bundle_tests tests/native_hash_bundle_tests.cpp)
    target_link_libraries(native_hash_bundle_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME native_hash_bundle COMMAND native_hash_bundle_tests -silent)
    qt_add_executable(native_hash_workflow_tests tests/native_hash_workflow_tests.cpp)
    target_link_libraries(native_hash_workflow_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME native_hash_workflow COMMAND native_hash_workflow_tests -silent)
    set_tests_properties(native_hash_workflow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
    qt_add_executable(hash_results_tests tests/hash_results_tests.cpp tests/layout_inspection.cpp)
    target_link_libraries(hash_results_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME hash_results COMMAND hash_results_tests -silent)
    qt_add_executable(benchmark_lifecycle_tests tests/benchmark_lifecycle_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(benchmark_lifecycle_tests PRIVATE src tests)
    target_link_libraries(benchmark_lifecycle_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME benchmark_lifecycle COMMAND benchmark_lifecycle_tests -silent)
    set_tests_properties(benchmark_lifecycle PROPERTIES TIMEOUT 30 RESOURCE_LOCK sevenzip_ui_artifacts ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/benchmark_lifecycle.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(hash_results PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports" TIMEOUT 30)
    qt_add_executable(extract_prompt_tests tests/extract_prompt_tests.cpp tests/layout_inspection.cpp)
    qt_add_executable(language_dialog_tests tests/language_dialog_tests.cpp tests/layout_inspection.cpp
        src/CPP/7zip/UI/FileManager/SplitDialog.cpp)
    target_include_directories(language_dialog_tests PRIVATE src tests)
    target_link_libraries(language_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME language_dialog COMMAND language_dialog_tests -silent)
    qt_add_executable(language_viewer_tests tests/language_viewer_tests.cpp
        tests/layout_inspection.cpp src/fileviewerdialog.cpp)
    target_include_directories(language_viewer_tests PRIVATE src tests)
    target_link_libraries(language_viewer_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME language_viewer COMMAND language_viewer_tests -silent)
    set_tests_properties(language_viewer PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports" TIMEOUT 30)
    set_tests_properties(language_dialog PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports" TIMEOUT 30)
    target_link_libraries(extract_prompt_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME extract_prompt COMMAND extract_prompt_tests -silent)
    set_tests_properties(extract_prompt PROPERTIES FIXTURES_SETUP ui_reference_extract_prompts)
    set_tests_properties(extract_prompt PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports"
        TIMEOUT 30)
    target_link_libraries(native_extract_callback_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME native_extract_callback COMMAND native_extract_callback_tests -silent)
    target_link_libraries(archive_update_callback_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME archive_update_callback COMMAND archive_update_callback_tests -silent)
    set_tests_properties(archive_update_callback PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    target_link_libraries(work_dir_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME work_dir COMMAND work_dir_tests -silent)
    qt_add_executable(context_menu_settings_tests tests/context_menu_settings_tests.cpp)
    target_include_directories(context_menu_settings_tests PRIVATE src)
    target_link_libraries(context_menu_settings_tests PRIVATE sevenzip_common Qt6::Test)
    add_test(NAME context_menu_settings COMMAND context_menu_settings_tests -silent)
    qt_add_executable(external_launch_tests tests/external_launch_tests.cpp tests/layout_inspection.cpp
        src/archivebackend.cpp src/archivebackend_listing.cpp src/archivebackend_validation.cpp
        src/archivebackend_mutations.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp)
    target_link_libraries(external_launch_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME external_launch COMMAND external_launch_tests -silent)
    set_tests_properties(external_launch PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_navigation_tests tests/panel_navigation_tests.cpp)
    target_link_libraries(panel_navigation_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME panel_navigation COMMAND panel_navigation_tests -silent)
    set_tests_properties(panel_navigation PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    target_link_libraries(open_callback_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME open_callback COMMAND open_callback_tests -silent)
    set_tests_properties(open_callback PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    qt_add_executable(archive_progress_tests tests/archive_progress_tests.cpp tests/layout_inspection.cpp
        src/CPP/7zip/UI/GUI/CompressDialog.cpp src/archivebackend.cpp
        src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp)
    qt_add_executable(archive_opening_tests tests/archive_opening_tests.cpp
        src/CPP/7zip/UI/GUI/CompressDialog.cpp src/archivebackend.cpp
        src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp)
    target_link_libraries(archive_opening_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME archive_opening COMMAND archive_opening_tests -silent)
    qt_add_executable(agent_capability_tests tests/agent_capability_tests.cpp
        src/CPP/7zip/UI/GUI/CompressDialog.cpp src/archivebackend.cpp
        src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp)
    target_link_libraries(agent_capability_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME agent_capability COMMAND agent_capability_tests -silent)
    set_tests_properties(archive_opening PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    target_link_libraries(archive_progress_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME archive_progress COMMAND archive_progress_tests -silent)
    set_tests_properties(archive_progress PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports"
        FIXTURES_SETUP ui_reference_archive_progress TIMEOUT 30)
    target_link_libraries(update_callback_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME update_callback COMMAND update_callback_tests -silent)
    set_tests_properties(update_callback PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    qt_add_executable(progress_cancel_tests tests/progress_cancel_tests.cpp
        tests/layout_inspection.cpp)
    target_link_libraries(progress_cancel_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME progress_cancel COMMAND progress_cancel_tests -silent)
    set_tests_properties(progress_cancel PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports"
        TIMEOUT 30)
    qt_add_executable(process_priority_tests tests/process_priority_tests.cpp)
    qt_add_executable(process_pause_tests tests/process_pause_tests.cpp)
    target_include_directories(process_pause_tests PRIVATE src)
    target_link_libraries(process_pause_tests PRIVATE sevenzip_common Qt6::Test)
    add_test(NAME process_pause COMMAND process_pause_tests -silent)
    set_tests_properties(process_pause PROPERTIES TIMEOUT 15)
    target_include_directories(process_priority_tests PRIVATE src)
    target_link_libraries(process_priority_tests PRIVATE sevenzip_common Qt6::Test)
    add_test(NAME process_priority COMMAND process_priority_tests -silent)
    set_tests_properties(process_priority PROPERTIES TIMEOUT 15)
    qt_add_executable(progress_priority_tests tests/progress_priority_tests.cpp
        tests/layout_inspection.cpp)
    target_link_libraries(progress_priority_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME progress_priority COMMAND progress_priority_tests -silent)
    set_tests_properties(progress_priority PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports"
        FIXTURES_SETUP ui_reference_progress_priority TIMEOUT 15)
    if(APPLE)
        qt_add_executable(progress_priority_failure_tests tests/progress_priority_failure_tests.cpp)
        target_link_libraries(progress_priority_failure_tests PRIVATE sevenzip_panel Qt6::Test)
        add_test(NAME progress_priority_failure COMMAND progress_priority_failure_tests -silent)
        set_tests_properties(progress_priority_failure PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)
        qt_add_executable(progress_taskbar_tests tests/progress_taskbar_tests.cpp
            tests/fixtures/dock_observer.mm tests/fixtures/dock_observer.h)
        target_link_libraries(progress_taskbar_tests PRIVATE sevenzip_panel Qt6::Test)
        add_test(NAME progress_taskbar COMMAND progress_taskbar_tests -silent)
        set_tests_properties(progress_taskbar PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            TIMEOUT 15)
        qt_add_executable(taskbar_progress_mac_tests tests/taskbar_progress_mac_tests.mm)
        target_link_libraries(taskbar_progress_mac_tests PRIVATE sevenzip_panel Qt6::Test)
        add_test(NAME taskbar_progress_mac COMMAND taskbar_progress_mac_tests)
        set_tests_properties(taskbar_progress_mac PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)
    endif()
    set_tests_properties(progress_sync PROPERTIES TIMEOUT 10)
    qt_add_executable(progress_format_tests tests/progress_format_tests.cpp)
    target_link_libraries(progress_format_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME progress_format COMMAND progress_format_tests -silent)
    qt_add_executable(progress_statistics_helpers_tests tests/progress_statistics_helpers_tests.cpp)
    target_include_directories(progress_statistics_helpers_tests PRIVATE src)
    target_link_libraries(progress_statistics_helpers_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME progress_statistics_helpers COMMAND progress_statistics_helpers_tests -silent)
    qt_add_executable(progress_dialog_tests tests/progress_dialog_tests.cpp tests/layout_inspection.cpp)
    target_link_libraries(progress_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME progress_dialog COMMAND progress_dialog_tests -silent)
    set_tests_properties(progress_dialog PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports"
        FIXTURES_SETUP ui_reference_progress TIMEOUT 40)
    add_test(NAME ui_parity_progress COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_ui_parity.py compare
        --reference ${CMAKE_CURRENT_SOURCE_DIR}/docs/ui-reference/progress-dialog.reference.json
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/docs/ui-reference/progress-dialog.contract.json
        --reports ${CMAKE_BINARY_DIR}/ui-reports
        --output ${CMAKE_BINARY_DIR}/ui-reports/progress-parity.json)
    set_tests_properties(ui_parity_progress PROPERTIES
        FIXTURES_REQUIRED "ui_reference_progress;ui_reference_progress_priority;ui_reference_progress_worker;ui_reference_archive_progress")
    qt_add_executable(file_find_tests tests/file_find_tests.cpp)
    target_link_libraries(file_find_tests PRIVATE sevenzip_filesystem Qt6::Test)
    add_test(NAME file_find COMMAND file_find_tests -silent)
    qt_add_executable(fs_folder_tests tests/fs_folder_tests.cpp)
    target_link_libraries(fs_folder_tests PRIVATE sevenzip_filesystem Qt6::Test)
    add_test(NAME fs_folder COMMAND fs_folder_tests -silent)
    qt_add_executable(volume_mount_tests tests/volume_mount_tests.cpp)
    target_link_libraries(volume_mount_tests PRIVATE sevenzip_filesystem Qt6::Test)
    add_test(NAME volume_mount COMMAND volume_mount_tests -silent)
    qt_add_executable(network_folder_tests tests/network_folder_tests.cpp)
    target_link_libraries(network_folder_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME network_folder COMMAND network_folder_tests -silent)
    set_tests_properties(network_folder PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(native_fs_operations_tests tests/native_fs_operations_tests.cpp)
    target_link_libraries(native_fs_operations_tests PRIVATE sevenzip_filesystem Qt6::Test)
    add_test(NAME native_fs_operations COMMAND native_fs_operations_tests -silent)
    qt_add_executable(location_folder_tests tests/location_folder_tests.cpp)
    target_link_libraries(location_folder_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME location_folder COMMAND location_folder_tests -silent)
    qt_add_executable(filesystem_scanner_tests tests/filesystem_scanner_tests.cpp
        src/filesystemscanner.cpp src/filesystemscanner.h)
    target_include_directories(filesystem_scanner_tests PRIVATE src)
    target_link_libraries(filesystem_scanner_tests PRIVATE sevenzip_folder_loader Qt6::Test)
    add_test(NAME filesystem_scanner COMMAND filesystem_scanner_tests -silent)
    qt_add_executable(panel_copy_info_tests tests/panel_copy_info_tests.cpp)
    target_link_libraries(panel_copy_info_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME panel_copy_info COMMAND panel_copy_info_tests -silent)
    set_tests_properties(panel_copy_info PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(copy_dialog_tests tests/copy_dialog_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(copy_dialog_tests PRIVATE src tests)
    target_link_libraries(copy_dialog_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_panel)
    add_test(NAME copy_dialog COMMAND copy_dialog_tests -silent)
    set_tests_properties(copy_dialog PROPERTIES FIXTURES_SETUP ui_reference_copy)
    set_tests_properties(copy_dialog PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports" TIMEOUT 30)
    qt_add_executable(archive_backend_tests tests/archive_backend_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h)
    qt_add_executable(archive_mutation_tests tests/archive_mutation_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h)
    target_include_directories(archive_backend_tests PRIVATE src)
    target_include_directories(archive_mutation_tests PRIVATE src tests)
    target_link_libraries(archive_backend_tests PRIVATE Qt6::Test)
    target_link_libraries(archive_mutation_tests PRIVATE Qt6::Test)
    qt_add_executable(ui_layout_tests tests/ui_layout_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(ui_layout_tests PRIVATE src)
    target_link_libraries(ui_layout_tests PRIVATE Qt6::Widgets Qt6::Test)
    qt_add_executable(ui_state_tests tests/ui_state_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    qt_add_executable(ui_operations_tests tests/ui_operations_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    qt_add_executable(ui_navigation_tests tests/ui_navigation_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    qt_add_executable(ui_archive_states_tests tests/ui_archive_states_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(ui_state_tests PRIVATE src)
    target_include_directories(ui_operations_tests PRIVATE src tests)
    target_include_directories(ui_navigation_tests PRIVATE src tests)
    target_include_directories(ui_archive_states_tests PRIVATE src tests)
    target_sources(ui_archive_states_tests PRIVATE tests/layout_inspection.cpp)
    target_link_libraries(ui_state_tests PRIVATE Qt6::Widgets Qt6::Test)
    target_link_libraries(ui_operations_tests PRIVATE Qt6::Widgets Qt6::Test)
    get_target_property(copy_same_window_sources ui_operations_tests SOURCES)
    list(REMOVE_ITEM copy_same_window_sources tests/ui_operations_tests.cpp)
    qt_add_executable(ui_copy_same_tests tests/ui_copy_same_tests.cpp ${copy_same_window_sources})
    target_include_directories(ui_copy_same_tests PRIVATE src tests)
    target_link_libraries(ui_copy_same_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME ui_copy_same COMMAND ui_copy_same_tests -silent)
    set_tests_properties(ui_copy_same PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/ui-copy-same-settings.ini")
    qt_add_executable(ui_update_modes_tests tests/ui_update_modes_tests.cpp ${copy_same_window_sources})
    target_include_directories(ui_update_modes_tests PRIVATE src tests)
    target_link_libraries(ui_update_modes_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME ui_update_modes COMMAND ui_update_modes_tests -silent)
    qt_add_executable(external_editors_ui_tests tests/external_editors_ui_tests.cpp
        tests/layout_inspection.cpp ${copy_same_window_sources})
    target_include_directories(external_editors_ui_tests PRIVATE src tests)
    target_link_libraries(external_editors_ui_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME external_editors_ui COMMAND external_editors_ui_tests -silent)
    qt_add_executable(verctrl_tests tests/verctrl_tests.cpp
        tests/layout_inspection.cpp ${copy_same_window_sources})
    target_include_directories(verctrl_tests PRIVATE src tests)
    target_link_libraries(verctrl_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME verctrl COMMAND verctrl_tests -silent)
    qt_add_executable(nested_edit_chain_tests tests/nested_edit_chain_tests.cpp
        tests/external_editor_fixture.cpp tests/layout_inspection.cpp ${copy_same_window_sources})
    target_include_directories(nested_edit_chain_tests PRIVATE src tests)
    target_link_libraries(nested_edit_chain_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME nested_edit_chain COMMAND nested_edit_chain_tests -silent)
    set_tests_properties(nested_edit_chain PROPERTIES TIMEOUT 120 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/nested_edit_chain.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(verctrl PROPERTIES TIMEOUT 90 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/verctrl.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(archive_external_edit_tests tests/archive_external_edit_tests.cpp
        tests/external_editor_fixture.cpp tests/layout_inspection.cpp ${copy_same_window_sources})
    target_include_directories(archive_external_edit_tests PRIVATE src tests)
    target_link_libraries(archive_external_edit_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME archive_external_edit COMMAND archive_external_edit_tests -silent)
    qt_add_executable(archive_external_security_tests tests/archive_external_security_tests.cpp
        tests/external_editor_fixture.cpp ${copy_same_window_sources})
    target_include_directories(archive_external_security_tests PRIVATE src tests)
    target_link_libraries(archive_external_security_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME archive_external_security COMMAND archive_external_security_tests -silent)
    set_tests_properties(archive_external_security PROPERTIES TIMEOUT 90 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/archive-external-security-settings.ini")
    qt_add_executable(archive_external_lifetime_tests tests/archive_external_lifetime_tests.cpp
        tests/external_editor_fixture.cpp src/archivebackend.cpp src/archivebackend_listing.cpp
        src/archivebackend_validation.cpp src/archivebackend_mutations.cpp
        src/CPP/7zip/UI/GUI/CompressDialog.cpp)
    target_include_directories(archive_external_lifetime_tests PRIVATE src tests)
    target_link_libraries(archive_external_lifetime_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME archive_external_lifetime COMMAND archive_external_lifetime_tests -silent)
    set_tests_properties(archive_external_lifetime PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/archive-external-lifetime-settings.ini")
    set_tests_properties(archive_external_edit PROPERTIES TIMEOUT 90 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/archive-external-edit-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(external_editors_ui PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/external-editors-ui-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(ui_archive_volumes_tests tests/ui_archive_volumes_tests.cpp ${copy_same_window_sources})
    target_include_directories(ui_archive_volumes_tests PRIVATE src tests)
    target_link_libraries(ui_archive_volumes_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME ui_archive_volumes COMMAND ui_archive_volumes_tests -silent)
    set_tests_properties(ui_archive_volumes PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/ui-archive-volumes-settings.ini")
    set_tests_properties(ui_update_modes PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/ui-update-modes-settings.ini")
    target_link_libraries(ui_navigation_tests PRIVATE Qt6::Widgets Qt6::Test)
    target_link_libraries(ui_archive_states_tests PRIVATE Qt6::Widgets Qt6::Test)
    qt_add_executable(flat_view_tests tests/flat_view_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    qt_add_executable(settings_tests tests/settings_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(settings_tests PRIVATE src)
    target_link_libraries(settings_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME settings COMMAND settings_tests -silent)
    set_tests_properties(settings PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/settings-tests.ini")
    qt_add_executable(properties_tests tests/properties_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(properties_tests PRIVATE src)
    target_link_libraries(properties_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME properties COMMAND properties_tests -silent)
    qt_add_executable(checksum_tests tests/checksum_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(checksum_tests PRIVATE src)
    target_link_libraries(checksum_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME checksum COMMAND checksum_tests -silent)
    set_tests_properties(checksum PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/checksum-tests.ini")
    set_tests_properties(properties PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/properties-tests.ini")
    target_include_directories(flat_view_tests PRIVATE src)
    target_sources(flat_view_tests PRIVATE tests/layout_inspection.cpp)
    target_link_libraries(flat_view_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME flat_view COMMAND flat_view_tests -silent)
    set_tests_properties(flat_view PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/flat-view-settings.ini"
    )
    add_test(NAME ui_state COMMAND ui_state_tests -silent)
    add_test(NAME ui_operations COMMAND ui_operations_tests -silent)
    add_test(NAME ui_navigation COMMAND ui_navigation_tests -silent)
    add_test(NAME ui_archive_states COMMAND ui_archive_states_tests -silent)
    foreach(suite ui_operations ui_navigation ui_archive_states)
        set_tests_properties(${suite} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/${suite}-settings.ini")
    endforeach()
    set_property(TEST ui_navigation APPEND PROPERTY ENVIRONMENT "SEVENZIP_SCREENSHOT_DIR=${CMAKE_BINARY_DIR}/ui-screenshots")
    set_tests_properties(ui_state PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SCREENSHOT_DIR=${CMAKE_BINARY_DIR}/ui-screenshots;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/ui-state-settings.ini"
    )
    add_test(NAME ui_layout COMMAND ui_layout_tests -silent)
    set_tests_properties(ui_layout PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SCREENSHOT_DIR=${CMAKE_BINARY_DIR}/ui-screenshots;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/ui-layout-settings.ini"
    )
    add_test(NAME archive_backend COMMAND archive_backend_tests)
    add_test(NAME archive_mutation COMMAND archive_mutation_tests)
    qt_add_executable(extract_dialog_tests tests/extract_dialog_tests.cpp
        src/CPP/7zip/UI/GUI/CompressDialog.cpp src/archivebackend.cpp
        src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp)
    target_include_directories(extract_dialog_tests PRIVATE src)
    target_link_libraries(extract_dialog_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_panel)
    add_test(NAME extract_dialog COMMAND extract_dialog_tests -silent)
    set_tests_properties(extract_dialog PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
    get_target_property(split_destination_sources extract_dialog_tests SOURCES)
    list(REMOVE_ITEM split_destination_sources tests/extract_dialog_tests.cpp)
    qt_add_executable(split_destination_tests tests/split_destination_tests.cpp ${split_destination_sources})
    target_include_directories(split_destination_tests PRIVATE src)
    target_link_libraries(split_destination_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_panel)
    add_test(NAME split_destination COMMAND split_destination_tests -silent)
    set_tests_properties(split_destination PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
    qt_add_executable(extraction_options_tests tests/extraction_options_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h)
    target_include_directories(extraction_options_tests PRIVATE src)
    target_link_libraries(extraction_options_tests PRIVATE Qt6::Test)
    add_test(NAME extraction_options COMMAND extraction_options_tests -silent)
    qt_add_executable(compression_tests tests/compression_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h)
    target_include_directories(compression_tests PRIVATE src)
    target_link_libraries(compression_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME compression COMMAND compression_tests -silent)
    get_target_property(compression_order_sources compression_tests SOURCES)
    list(REMOVE_ITEM compression_order_sources tests/compression_tests.cpp)
    qt_add_executable(compression_advanced_tests tests/compression_advanced_tests.cpp
        tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(compression_advanced_tests PRIVATE src tests)
    target_link_libraries(compression_advanced_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_panel)
    add_test(NAME compression_advanced COMMAND compression_advanced_tests -silent)
    set_tests_properties(compression_advanced PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(archive_volume_creation_tests tests/archive_volume_creation_tests.cpp
        tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(archive_volume_creation_tests PRIVATE src tests)
    target_link_libraries(archive_volume_creation_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_panel)
    add_test(NAME archive_volume_creation COMMAND archive_volume_creation_tests -silent)
    set_tests_properties(archive_volume_creation PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(archive_creation_lifecycle_tests tests/archive_creation_lifecycle_tests.cpp
        src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp
        src/archivebackend_listing.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp)
    target_include_directories(archive_creation_lifecycle_tests PRIVATE src)
    target_link_libraries(archive_creation_lifecycle_tests PRIVATE Qt6::Test sevenzip_panel)
    add_test(NAME archive_creation_lifecycle COMMAND archive_creation_lifecycle_tests -silent)
    set_tests_properties(archive_creation_lifecycle PROPERTIES TIMEOUT 60)
    qt_add_executable(compression_path_tests tests/compression_path_tests.cpp tests/layout_inspection.cpp ${compression_order_sources})
    qt_add_executable(sfx_options_tests tests/sfx_options_tests.cpp ${compression_order_sources})
    add_library(external_codec_fixture MODULE tests/external_codec_fixture.cpp
        third_party/7zip/CPP/7zip/Compress/CodecExports.cpp
        third_party/7zip/CPP/7zip/Compress/DllExportsCompress.cpp
        third_party/7zip/CPP/7zip/Compress/CopyCoder.cpp
        third_party/7zip/CPP/Windows/PropVariant.cpp
        third_party/7zip/CPP/Common/MyWindows.cpp
        third_party/7zip/CPP/Common/MyString.cpp
        third_party/7zip/CPP/Common/IntToString.cpp
        third_party/7zip/CPP/Common/StringConvert.cpp
        third_party/7zip/CPP/Common/UTFConvert.cpp
        third_party/7zip/C/Alloc.c third_party/7zip/C/CpuArch.c)
    target_include_directories(external_codec_fixture PRIVATE third_party/7zip/CPP)
    target_link_libraries(external_codec_fixture PRIVATE Threads::Threads)
    set_target_properties(external_codec_fixture PROPERTIES AUTOMOC OFF AUTOUIC OFF
        PREFIX "")
    if(UNIX AND NOT APPLE)
        target_link_options(external_codec_fixture PRIVATE "-Wl,-Bsymbolic")
    endif()
    if(WIN32)
        target_sources(external_codec_fixture PRIVATE third_party/7zip/CPP/7zip/Compress/Codec.def)
        target_link_libraries(external_codec_fixture PRIVATE oleaut32)
    endif()
    qt_add_executable(external_codec_tests tests/external_codec_tests.cpp ${compression_order_sources})
    add_library(incompatible_codec_fixture MODULE tests/incompatible_codec_fixture.cpp)
    target_include_directories(incompatible_codec_fixture PRIVATE third_party/7zip/CPP)
    set_target_properties(incompatible_codec_fixture PROPERTIES AUTOMOC OFF AUTOUIC OFF
        PREFIX "")
    add_library(external_format_fixture MODULE tests/external_format_fixture.cpp
        third_party/7zip/CPP/Common/MyWindows.cpp)
    target_include_directories(external_format_fixture PRIVATE third_party/7zip/CPP)
    target_link_libraries(external_format_fixture PRIVATE ${CMAKE_DL_LIBS})
    set_target_properties(external_format_fixture PROPERTIES AUTOMOC OFF AUTOUIC OFF
        PREFIX "")
    if(WIN32)
        target_sources(incompatible_codec_fixture PRIVATE tests/incompatible_codec_fixture.def)
        target_sources(external_format_fixture PRIVATE tests/external_format_fixture.def)
        target_link_libraries(external_format_fixture PRIVATE oleaut32)
    elseif(NOT APPLE)
        target_link_options(external_format_fixture PRIVATE "-Wl,-Bsymbolic")
    endif()
    target_include_directories(external_codec_tests PRIVATE src)
    target_link_libraries(external_codec_tests PRIVATE sevenzip_panel Qt6::Test)
    target_compile_definitions(external_codec_tests PRIVATE
        "SEVENZIP_EXTERNAL_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_EXTERNAL_FIXTURE=\"$<TARGET_FILE:external_codec_fixture>\""
        "SEVENZIP_EXTERNAL_FORMAT_FIXTURE=\"$<TARGET_FILE:external_format_fixture>\""
        "SEVENZIP_INCOMPATIBLE_FIXTURE=\"$<TARGET_FILE:incompatible_codec_fixture>\"")
    add_dependencies(external_codec_tests external_codec_fixture incompatible_codec_fixture external_format_fixture)
    add_test(NAME external_codecs COMMAND external_codec_tests -silent)
    set_tests_properties(external_codecs PROPERTIES TIMEOUT 60)
    target_include_directories(sfx_options_tests PRIVATE src)
    target_link_libraries(sfx_options_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME sfx_options COMMAND sfx_options_tests -silent)
    qt_add_executable(sfx_creation_tests tests/sfx_creation_tests.cpp tests/layout_inspection.cpp ${compression_order_sources})
    set(sevenzip_sfx_fixture_name "7zCon.sfx")
    if(WIN32)
        set(sevenzip_sfx_fixture_name "7z.sfx")
    endif()
    target_include_directories(sfx_creation_tests PRIVATE src tests)
    target_link_libraries(sfx_creation_tests PRIVATE sevenzip_panel Qt6::Test)
    target_compile_definitions(sfx_creation_tests PRIVATE
        "SEVENZIP_SFX_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/build/sfx/${sevenzip_sfx_fixture_name}\"")
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(sfx_creation_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME sfx_creation COMMAND sfx_creation_tests -silent)
    qt_add_executable(sfx_lifecycle_tests tests/sfx_lifecycle_tests.cpp ${compression_order_sources})
    target_include_directories(sfx_lifecycle_tests PRIVATE src)
    target_link_libraries(sfx_lifecycle_tests PRIVATE sevenzip_panel Qt6::Test)
    target_compile_definitions(sfx_lifecycle_tests PRIVATE
        "SEVENZIP_SFX_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/build/sfx/${sevenzip_sfx_fixture_name}\"")
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(sfx_lifecycle_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME sfx_lifecycle COMMAND sfx_lifecycle_tests -silent)
    set_tests_properties(sfx_lifecycle PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    set_tests_properties(sfx_creation PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    target_include_directories(compression_path_tests PRIVATE src tests)
    target_link_libraries(compression_path_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME compression_path COMMAND compression_path_tests -silent)
    qt_add_executable(codec_creation_ui_tests tests/codec_creation_ui_tests.cpp tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(codec_creation_ui_tests PRIVATE src tests)
    target_link_libraries(codec_creation_ui_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common sevenzip_panel)
    add_test(NAME codec_creation_ui COMMAND codec_creation_ui_tests -silent)
    set_tests_properties(codec_creation_ui PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(compression_path PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(compression_encryption_tests tests/compression_encryption_tests.cpp tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(compression_encryption_tests PRIVATE src tests)
    target_link_libraries(compression_encryption_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME compression_encryption COMMAND compression_encryption_tests -silent)
    set_tests_properties(compression_encryption PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(compression_order_tests tests/compression_order_tests.cpp ${compression_order_sources})
    target_include_directories(compression_order_tests PRIVATE src)
    target_link_libraries(compression_order_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME compression_order COMMAND compression_order_tests -silent)
    qt_add_executable(compression_dictionary_tests tests/compression_dictionary_tests.cpp
        tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(compression_dictionary_tests PRIVATE src)
    target_link_libraries(compression_dictionary_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME compression_dictionary COMMAND compression_dictionary_tests -silent)
    set_tests_properties(compression_dictionary PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(compression_memory_tests tests/compression_memory_tests.cpp
        tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(compression_memory_tests PRIVATE src)
    target_link_libraries(compression_memory_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME compression_memory COMMAND compression_memory_tests -silent)
    set_tests_properties(compression_memory PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(memory_limit_tests tests/memory_limit_tests.cpp)
    target_include_directories(memory_limit_tests PRIVATE src)
    target_link_libraries(memory_limit_tests PRIVATE Qt6::Test sevenzip_common)
    add_test(NAME memory_limit COMMAND memory_limit_tests -silent)
    qt_add_executable(compression_limit_tests tests/compression_limit_tests.cpp
        tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(compression_limit_tests PRIVATE src)
    target_link_libraries(compression_limit_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME compression_limit COMMAND compression_limit_tests -silent)
    set_tests_properties(compression_limit PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(compression_preferences_tests tests/compression_preferences_tests.cpp ${compression_order_sources})
    target_include_directories(compression_preferences_tests PRIVATE src)
    target_link_libraries(compression_preferences_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME compression_preferences COMMAND compression_preferences_tests -silent)
    set_tests_properties(compression_preferences PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(compression_resource_tests tests/compression_resource_tests.cpp ${compression_order_sources})
    target_include_directories(compression_resource_tests PRIVATE src)
    target_link_libraries(compression_resource_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME compression_resources COMMAND compression_resource_tests -silent)
    set_tests_properties(compression_resources PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(compression_solid_tests tests/compression_solid_tests.cpp
        tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(compression_solid_tests PRIVATE src)
    target_link_libraries(compression_solid_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME compression_solid COMMAND compression_solid_tests -silent)
    foreach(target IN ITEMS compression_tests compression_path_tests compression_encryption_tests
        compression_order_tests compression_dictionary_tests compression_memory_tests
        compression_limit_tests compression_preferences_tests compression_resource_tests
        compression_solid_tests)
        target_link_libraries(${target} PRIVATE sevenzip_panel)
    endforeach()
    set_tests_properties(compression_solid PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")

    set_tests_properties(compression_order PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    set_tests_properties(compression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    foreach(suite compression compression_order compression_dictionary compression_memory compression_resources compression_solid compression_limit)
        set_property(TEST ${suite} APPEND PROPERTY ENVIRONMENT "SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/${suite}-settings.ini")
    endforeach()
    qt_add_executable(file_viewer_tests tests/file_viewer_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(file_viewer_tests PRIVATE src)
    target_link_libraries(file_viewer_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME file_viewer COMMAND file_viewer_tests -silent)
    set_tests_properties(file_viewer PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/file-viewer-settings.ini")
    qt_add_executable(nested_archive_tests tests/nested_archive_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/fileviewerdialog.cpp src/fileviewerdialog.h src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h src/propertiesdialog.cpp src/propertiesdialog.h src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(nested_archive_tests PRIVATE src)
    target_link_libraries(nested_archive_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME nested_archive COMMAND nested_archive_tests -silent)
    set_tests_properties(nested_archive PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/nested-archive-settings.ini")
    get_target_property(native_navigation_sources nested_archive_tests SOURCES)
    list(REMOVE_ITEM native_navigation_sources tests/nested_archive_tests.cpp)
    qt_add_executable(native_navigation_window_tests tests/native_navigation_window_tests.cpp
        ${native_navigation_sources})
    target_include_directories(native_navigation_window_tests PRIVATE src)
    target_link_libraries(native_navigation_window_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME native_navigation_window COMMAND native_navigation_window_tests -silent)
    set_tests_properties(native_navigation_window PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native-navigation-settings.ini")
    qt_add_executable(file_transfer_tests tests/file_transfer_tests.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h)
    target_include_directories(file_transfer_tests PRIVATE src)
    target_link_libraries(file_transfer_tests PRIVATE Qt6::Test sevenzip_filesystem)
    add_test(NAME file_transfer COMMAND file_transfer_tests -silent)
    qt_add_executable(file_transfer_direct_tests tests/file_transfer_direct_tests.cpp
        src/filetransferoperation.cpp src/filetransferoperation.h)
    target_include_directories(file_transfer_direct_tests PRIVATE src)
    target_link_libraries(file_transfer_direct_tests PRIVATE Qt6::Test sevenzip_filesystem)
    add_test(NAME file_transfer_direct COMMAND file_transfer_direct_tests -silent)
    qt_add_executable(language_file_transfer_tests tests/language_file_transfer_tests.cpp
        src/filetransferoperation.cpp src/filetransferoperation.h)
    target_include_directories(language_file_transfer_tests PRIVATE src)
    target_link_libraries(language_file_transfer_tests PRIVATE Qt6::Test sevenzip_filesystem)
    add_test(NAME language_file_transfer COMMAND language_file_transfer_tests -silent)
    set_tests_properties(language_file_transfer PROPERTIES TIMEOUT 60)
    qt_add_executable(fs_folder_copy_tests tests/fs_folder_copy_tests.cpp)
    target_include_directories(fs_folder_copy_tests PRIVATE src)
    target_link_libraries(fs_folder_copy_tests PRIVATE sevenzip_filesystem Qt6::Test)
    add_test(NAME fs_folder_copy COMMAND fs_folder_copy_tests -silent)
    get_target_property(panel_sort_sources ui_state_tests SOURCES)
    list(REMOVE_ITEM panel_sort_sources tests/ui_state_tests.cpp)
    qt_add_executable(command_policy_runtime_tests tests/command_policy_runtime_tests.cpp ${panel_sort_sources})
    target_include_directories(command_policy_runtime_tests PRIVATE src tests)
    target_link_libraries(command_policy_runtime_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(command_policy_runtime_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME command_policy_runtime COMMAND command_policy_runtime_tests -silent)
    set_tests_properties(command_policy_runtime PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/command-policy-settings.ini")
    qt_add_executable(root_navigation_tests tests/root_navigation_tests.cpp
        tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(root_navigation_tests PRIVATE src tests)
    target_link_libraries(root_navigation_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(root_navigation_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME root_navigation COMMAND root_navigation_tests -silent)
    set_tests_properties(root_navigation PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/root-navigation-settings.ini")
    qt_add_executable(bookmark_tests tests/bookmark_tests.cpp ${panel_sort_sources})
    target_include_directories(bookmark_tests PRIVATE src tests)
    target_link_libraries(bookmark_tests PRIVATE sevenzip_panel sevenzip_filesystem Qt6::Test)
    add_test(NAME bookmarks COMMAND bookmark_tests -silent)
    set_tests_properties(bookmarks PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/bookmark-settings.ini")
    qt_add_executable(panel_focus_tests tests/panel_focus_tests.cpp ${panel_sort_sources})
    target_include_directories(panel_focus_tests PRIVATE src tests)
    target_link_libraries(panel_focus_tests PRIVATE sevenzip_panel sevenzip_filesystem Qt6::Test)
    add_test(NAME panel_focus COMMAND panel_focus_tests -silent)
    set_tests_properties(panel_focus PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-focus-settings.ini")
    qt_add_executable(panel_selected_state_tests tests/panel_selected_state_tests.cpp ${panel_sort_sources})
    target_include_directories(panel_selected_state_tests PRIVATE src tests)
    target_link_libraries(panel_selected_state_tests PRIVATE sevenzip_panel sevenzip_filesystem Qt6::Test)
    target_sources(panel_selected_state_tests PRIVATE tests/layout_inspection.cpp)
    add_test(NAME panel_selected_state COMMAND panel_selected_state_tests -silent)
    set_tests_properties(panel_selected_state PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-selected-state-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_runtime_tests tests/panel_runtime_tests.cpp ${panel_sort_sources})
    target_include_directories(panel_runtime_tests PRIVATE src tests)
    target_link_libraries(panel_runtime_tests PRIVATE sevenzip_panel sevenzip_filesystem Qt6::Test)
    target_sources(panel_runtime_tests PRIVATE tests/layout_inspection.cpp)
    add_test(NAME panel_runtime COMMAND panel_runtime_tests -silent)
    set_tests_properties(panel_runtime PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-runtime-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(properties_runtime_tests tests/properties_runtime_tests.cpp ${panel_sort_sources})
    target_include_directories(properties_runtime_tests PRIVATE src tests)
    target_link_libraries(properties_runtime_tests PRIVATE sevenzip_panel sevenzip_filesystem Qt6::Test)
    target_sources(properties_runtime_tests PRIVATE tests/layout_inspection.cpp)
    add_test(NAME properties_runtime COMMAND properties_runtime_tests -silent)
    set_tests_properties(properties_runtime PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/properties-runtime-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(archive_selection_runtime_tests tests/archive_selection_runtime_tests.cpp ${panel_sort_sources})
    target_include_directories(archive_selection_runtime_tests PRIVATE src tests)
    target_link_libraries(archive_selection_runtime_tests PRIVATE sevenzip_panel sevenzip_filesystem Qt6::Test)
    add_test(NAME archive_selection_runtime COMMAND archive_selection_runtime_tests -silent)
    set_tests_properties(archive_selection_runtime PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/archive-selection-runtime-settings.ini")
    qt_add_executable(property_formatting_tests tests/property_formatting_tests.cpp)
    qt_add_executable(binding_lifecycle_tests tests/binding_lifecycle_tests.cpp ${panel_sort_sources})
    qt_add_executable(app_lifecycle_window_tests tests/app_lifecycle_window_tests.cpp ${panel_sort_sources})
    target_include_directories(app_lifecycle_window_tests PRIVATE src tests)
    target_link_libraries(app_lifecycle_window_tests PRIVATE sevenzip_panel sevenzip_filesystem Qt6::Test)
    add_test(NAME app_lifecycle_window COMMAND app_lifecycle_window_tests -silent)
    set_tests_properties(app_lifecycle_window PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/app-lifecycle-window-settings.ini")
    qt_add_executable(app_lifecycle_tests tests/app_lifecycle_tests.cpp)
    qt_add_executable(native_toolbar_state_tests tests/native_toolbar_state_tests.cpp)
    target_link_libraries(native_toolbar_state_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME native_toolbar_state COMMAND native_toolbar_state_tests -silent)
    set_tests_properties(native_toolbar_state PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(window_info_tests tests/window_info_tests.cpp)
    qt_add_executable(file_menu_policy_tests tests/file_menu_policy_tests.cpp)
    target_include_directories(file_menu_policy_tests PRIVATE src)
    target_link_libraries(file_menu_policy_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME file_menu_policy COMMAND file_menu_policy_tests -silent)
    qt_add_executable(native_command_dispatch_tests tests/native_command_dispatch_tests.cpp)
    target_include_directories(native_command_dispatch_tests PRIVATE src)
    target_link_libraries(native_command_dispatch_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME native_command_dispatch COMMAND native_command_dispatch_tests -silent)
    qt_add_executable(native_open_commands_tests tests/native_open_commands_tests.cpp)
    target_link_libraries(native_open_commands_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME native_open_commands COMMAND native_open_commands_tests -silent)
    set_tests_properties(native_open_commands PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)
    target_include_directories(window_info_tests PRIVATE src)
    target_link_libraries(window_info_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME window_info COMMAND window_info_tests -silent)
    target_include_directories(app_lifecycle_tests PRIVATE src)
    target_link_libraries(app_lifecycle_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME app_lifecycle COMMAND app_lifecycle_tests -silent)
    set_tests_properties(app_lifecycle PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    target_include_directories(binding_lifecycle_tests PRIVATE src tests)
    target_link_libraries(binding_lifecycle_tests PRIVATE sevenzip_panel sevenzip_filesystem Qt6::Test)
    add_test(NAME binding_lifecycle COMMAND binding_lifecycle_tests -silent)
    set_tests_properties(binding_lifecycle PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/binding-lifecycle-settings.ini")
    qt_add_executable(filesystem_selection_runtime_tests tests/filesystem_selection_runtime_tests.cpp ${panel_sort_sources})
    target_include_directories(filesystem_selection_runtime_tests PRIVATE src tests)
    target_link_libraries(filesystem_selection_runtime_tests PRIVATE sevenzip_panel sevenzip_filesystem Qt6::Test)
    add_test(NAME filesystem_selection_runtime COMMAND filesystem_selection_runtime_tests -silent)
    set_tests_properties(filesystem_selection_runtime PROPERTIES TIMEOUT 45 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/filesystem-selection-runtime-settings.ini")
    target_link_libraries(property_formatting_tests PRIVATE sevenzip_filesystem Qt6::Test)
    add_test(NAME property_formatting COMMAND property_formatting_tests -silent)

    qt_add_executable(property_classification_tests tests/property_classification_tests.cpp)
    target_link_libraries(property_classification_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME property_classification COMMAND property_classification_tests -silent)

    qt_add_executable(property_name_tests tests/property_name_tests.cpp)
    target_link_libraries(property_name_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME property_name COMMAND property_name_tests -silent)

    qt_add_executable(archive_error_properties_tests tests/archive_error_properties_tests.cpp)
    target_link_libraries(archive_error_properties_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME archive_error_properties COMMAND archive_error_properties_tests -silent)

    qt_add_executable(panel_property_tests tests/panel_property_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(panel_property_tests PRIVATE tests)
    target_link_libraries(panel_property_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME panel_property COMMAND panel_property_tests -silent)
    set_tests_properties(panel_property PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")

    qt_add_executable(archive_property_catalog_tests tests/archive_property_catalog_tests.cpp)
    target_link_libraries(archive_property_catalog_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME archive_property_catalog COMMAND archive_property_catalog_tests -silent)
    add_test(NAME resource_strings_generator COMMAND "${CMAKE_COMMAND}"
        "-DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/resource-strings-tests"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/rc_strings_tests.cmake")
    qt_add_executable(volume_workflow_tests tests/volume_workflow_tests.cpp
        tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(volume_workflow_tests PRIVATE src tests)
    target_link_libraries(volume_workflow_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME volume_workflow COMMAND volume_workflow_tests -silent)
    set_tests_properties(volume_workflow PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/volume-workflow-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_sort_tests tests/panel_sort_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_sort_tests PRIVATE src)
    target_link_libraries(panel_sort_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME panel_sort COMMAND panel_sort_tests -silent)
    qt_add_executable(panel_columns_tests tests/panel_columns_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_columns_tests PRIVATE src tests)
    target_link_libraries(panel_columns_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_columns_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_columns COMMAND panel_columns_tests -silent)
    set_tests_properties(panel_columns PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-columns-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_auto_refresh_tests tests/panel_auto_refresh_tests.cpp ${panel_sort_sources})
    target_include_directories(panel_auto_refresh_tests PRIVATE src tests)
    target_link_libraries(panel_auto_refresh_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_auto_refresh_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_auto_refresh COMMAND panel_auto_refresh_tests -silent)
    set_tests_properties(panel_auto_refresh PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-auto-refresh-settings.ini")
    qt_add_executable(list_settings_runtime_tests tests/list_settings_runtime_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(list_settings_runtime_tests PRIVATE src tests)
    target_link_libraries(list_settings_runtime_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(list_settings_runtime_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME list_settings_runtime COMMAND list_settings_runtime_tests -silent)
    set_tests_properties(list_settings_runtime PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/list-settings-runtime.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(combine_name_tests tests/combine_name_tests.cpp ${panel_sort_sources})
    target_include_directories(combine_name_tests PRIVATE src)
    target_link_libraries(combine_name_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME combine_names COMMAND combine_name_tests -silent)
    set_tests_properties(combine_names PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/combine-name-settings.ini")
    qt_add_executable(panel_key_tests tests/panel_key_tests.cpp ${panel_sort_sources})
    target_sources(panel_key_tests PRIVATE tests/layout_inspection.cpp)
    target_include_directories(panel_key_tests PRIVATE src)
    target_link_libraries(panel_key_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME panel_key COMMAND panel_key_tests -silent)
    qt_add_executable(main_shell_tests tests/main_shell_tests.cpp tests/layout_inspection.cpp
        ${panel_sort_sources})
    target_include_directories(main_shell_tests PRIVATE src)
    target_link_libraries(main_shell_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME main_shell COMMAND main_shell_tests -silent)
    set_tests_properties(main_shell PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/main-shell-test-settings.ini" TIMEOUT 30)
    qt_add_executable(panel_shell_tests tests/panel_shell_tests.cpp tests/layout_inspection.cpp
        ${panel_sort_sources})
    target_include_directories(panel_shell_tests PRIVATE src)
    target_link_libraries(panel_shell_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    add_test(NAME panel_shell COMMAND panel_shell_tests -silent)
    set_tests_properties(panel_shell PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-shell-test-settings.ini" TIMEOUT 30)
    set_tests_properties(panel_key PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-key-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(panel_sort PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-sort-settings.ini;SEVENZIP_SCREENSHOT_DIR=${CMAKE_BINARY_DIR}/ui-screenshots")
    qt_add_executable(panel_selection_tests tests/panel_selection_tests.cpp
        src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h
        )
    target_include_directories(panel_selection_tests PRIVATE src)
    target_link_libraries(panel_selection_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME panel_selection COMMAND panel_selection_tests -silent)
    set_tests_properties(panel_selection PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(archive_proxy_tests tests/archive_proxy_tests.cpp src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/Agent.cpp)
    target_include_directories(archive_proxy_tests PRIVATE src)
    target_link_libraries(archive_proxy_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME archive_proxy COMMAND archive_proxy_tests -silent)
    qt_add_executable(agent_folder_tests tests/agent_folder_tests.cpp src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/Agent.cpp)
    target_include_directories(agent_folder_tests PRIVATE src)
    target_link_libraries(agent_folder_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME agent_folder COMMAND agent_folder_tests -silent)
    qt_add_executable(agent_real_indices_tests tests/agent_real_indices_tests.cpp)
    target_link_libraries(agent_real_indices_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME agent_real_indices COMMAND agent_real_indices_tests -silent)
    qt_add_executable(panel_path_tests tests/panel_path_tests.cpp)
    target_link_libraries(panel_path_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME panel_path COMMAND panel_path_tests -silent)
    qt_add_executable(volume_operations_tests tests/volume_operations_tests.cpp src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(volume_operations_tests PRIVATE src)
    target_link_libraries(volume_operations_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME volume_operations COMMAND volume_operations_tests -silent)
    set_tests_properties(volume_operations PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(ui_geometry_tests tests/ui_geometry_tests.cpp tests/layout_inspection.cpp
        ${compression_order_sources})
    target_include_directories(ui_geometry_tests PRIVATE src tests)
    target_link_libraries(ui_geometry_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_panel)
    add_test(NAME ui_geometry COMMAND ui_geometry_tests -silent)
    set_tests_properties(ui_geometry PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    foreach(target IN ITEMS ui_layout_tests extract_dialog_tests compression_preferences_tests compression_tests)
        target_sources(${target} PRIVATE tests/layout_inspection.cpp)
    endforeach()
    target_sources(properties_tests PRIVATE tests/layout_inspection.cpp)
    qt_add_executable(fm_settings_tests tests/fm_settings_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(fm_settings_tests PRIVATE src tests)
    target_link_libraries(fm_settings_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME fm_settings COMMAND fm_settings_tests -silent)
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
    find_program(SEVENZIP_TEST_BASH NAMES bash)
    if(SEVENZIP_TEST_BASH)
        add_test(NAME agent_output COMMAND ${SEVENZIP_TEST_BASH}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/agent_output_test.sh)
    endif()
    add_test(NAME application_starts COMMAND sevenzip_desktop --smoke-test)
    set_tests_properties(application_starts PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/application-smoke-settings.ini"
    )
endif()

foreach(target IN ITEMS sevenzip_desktop archive_proxy_tests archive_backend_tests archive_mutation_tests ui_layout_tests ui_state_tests ui_operations_tests ui_copy_same_tests ui_update_modes_tests ui_archive_volumes_tests external_editors_ui_tests verctrl_tests nested_edit_chain_tests archive_external_edit_tests archive_external_security_tests ui_navigation_tests ui_archive_states_tests flat_view_tests settings_tests properties_tests checksum_tests extraction_options_tests compression_tests file_viewer_tests nested_archive_tests volume_operations_tests)
    if(TARGET ${target})
        target_link_libraries(${target} PRIVATE sevenzip_common)
    endif()
endforeach()

# Reuse the complete MainWindow fixture component, including its narrow adapters.
if(BUILD_TESTING)
    get_target_property(folder_size_view_sources ui_operations_tests SOURCES)
    list(REMOVE_ITEM folder_size_view_sources tests/ui_operations_tests.cpp)
    get_target_property(folder_size_view_libraries ui_operations_tests LINK_LIBRARIES)
    qt_add_executable(folder_size_view_tests tests/folder_size_view_tests.cpp
        tests/layout_inspection.cpp src/mainwindow_selectionpaths.cpp
        ${folder_size_view_sources})
    target_include_directories(folder_size_view_tests PRIVATE src tests)
    target_link_libraries(folder_size_view_tests PRIVATE ${folder_size_view_libraries}
        sevenzip_location sevenzip_folder_loader)
    add_test(NAME folder_size_view COMMAND folder_size_view_tests -silent)
    set_tests_properties(folder_size_view PROPERTIES TIMEOUT 90 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/folder_size_view.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(drag_drop_ui_tests tests/drag_drop_ui_tests.cpp
        tests/layout_inspection.cpp src/mainwindow_selectionpaths.cpp
        ${folder_size_view_sources})
    target_include_directories(drag_drop_ui_tests PRIVATE src tests)
    target_link_libraries(drag_drop_ui_tests PRIVATE ${folder_size_view_libraries}
        sevenzip_location sevenzip_folder_loader)
    add_test(NAME drag_drop_ui COMMAND drag_drop_ui_tests -silent)
    set_tests_properties(drag_drop_ui PROPERTIES TIMEOUT 60 RESOURCE_LOCK sevenzip_ui_artifacts ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/drag_drop_ui.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(drag_drop_add_ui_tests tests/drag_drop_add_ui_tests.cpp
        src/mainwindow_selectionpaths.cpp ${folder_size_view_sources})
    target_include_directories(drag_drop_add_ui_tests PRIVATE src tests)
    target_link_libraries(drag_drop_add_ui_tests PRIVATE ${folder_size_view_libraries}
        sevenzip_location sevenzip_folder_loader)
    add_test(NAME drag_drop_add_ui COMMAND drag_drop_add_ui_tests -silent)
    set_tests_properties(drag_drop_add_ui PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/drag_drop_add_ui.ini")
endif()

# Every target compiling the engine owner needs this typed source-update transport.
get_property(engine_consumer_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(engine_consumer IN LISTS engine_consumer_targets)
    get_target_property(engine_consumer_sources ${engine_consumer} SOURCES)
    if(NOT engine_consumer STREQUAL "sevenzip_panel" AND
       "src/CPP/7zip/UI/GUI/CompressDialog.cpp" IN_LIST engine_consumer_sources)
        target_link_libraries(${engine_consumer} PRIVATE sevenzip_panel)
    endif()
    if(NOT engine_consumer STREQUAL "sevenzip_panel" AND
       "src/CPP/7zip/UI/FileManager/SplitUtils.cpp" IN_LIST engine_consumer_sources)
        list(REMOVE_ITEM engine_consumer_sources src/CPP/7zip/UI/FileManager/SplitUtils.cpp)
        set_property(TARGET ${engine_consumer} PROPERTY SOURCES ${engine_consumer_sources})
        target_link_libraries(${engine_consumer} PRIVATE sevenzip_panel)
    endif()
    if("src/mainwindow.cpp" IN_LIST engine_consumer_sources)
        target_sources(${engine_consumer} PRIVATE src/qtapppresentation.cpp src/qtapppresentation.h
            src/mainwindow_propertyinvalidation.cpp
            src/mainwindow_close.cpp src/qtfilemanagerwindowservices.h
            src/qtfilemanagercommands.cpp src/qtfilemanagercommands.h
            src/qtfilemanagercommandworkflows.cpp src/mainwindow_root.cpp
            src/qtfilemanagerdrag.cpp src/qtfilemanagerdrag.h src/qtfilemanagerdragpresentation.cpp
            src/qtfilemanagerdraginput.cpp
            src/mainwindow_twopanel.cpp
            src/qtdialogscreenservices.cpp src/qtdialogscreenservices.h
            src/qtfilemenuprojection.cpp src/qtfilemenuprojection.h)
    endif()
    if("src/archivebackend.cpp" IN_LIST engine_consumer_sources)
        target_sources(${engine_consumer} PRIVATE src/archivebackend_sourceupdate.cpp src/archivebackend_sourceplan.cpp
            src/archivebackend_sourceextract.cpp src/archivebackend_volumes.cpp
            src/archivebackend_creation.cpp src/archivebackend_hash.cpp)
        target_link_libraries(${engine_consumer} PRIVATE sevenzip_filesystem)
    endif()
    if("src/createarchivedialog.cpp" IN_LIST engine_consumer_sources)
        target_sources(${engine_consumer} PRIVATE src/createarchivedialog_language.cpp)
        target_sources(${engine_consumer} PRIVATE src/createarchivedialog_volumes.cpp
            src/createarchivedialog_advanced.cpp src/createarchivedialog_formats.cpp
            src/createarchivedialog_sfx.cpp)
    endif()
endforeach()

# Window runtime uses the authoritative source-owned archive location factory.
if(TARGET flat_view_tests)
    set_property(TEST flat_view APPEND PROPERTY ENVIRONMENT
        "SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
endif()
foreach(target IN ITEMS sevenzip_desktop ui_layout_tests ui_state_tests ui_operations_tests ui_copy_same_tests ui_update_modes_tests ui_archive_volumes_tests external_editors_ui_tests verctrl_tests nested_edit_chain_tests archive_external_edit_tests archive_external_security_tests
    ui_navigation_tests ui_archive_states_tests flat_view_tests settings_tests properties_tests
    checksum_tests file_viewer_tests nested_archive_tests native_navigation_window_tests panel_sort_tests combine_name_tests
    panel_key_tests main_shell_tests panel_shell_tests volume_workflow_tests bookmark_tests panel_focus_tests panel_selected_state_tests panel_runtime_tests properties_runtime_tests archive_selection_runtime_tests filesystem_selection_runtime_tests binding_lifecycle_tests app_lifecycle_window_tests)
    if(TARGET ${target})
        target_link_libraries(${target} PRIVATE sevenzip_location sevenzip_folder_loader)
        target_sources(${target} PRIVATE src/mainwindow_selectionpaths.cpp)
    endif()
endforeach()

foreach(target IN ITEMS sevenzip_desktop ui_layout_tests ui_state_tests ui_operations_tests ui_copy_same_tests ui_update_modes_tests ui_archive_volumes_tests external_editors_ui_tests verctrl_tests nested_edit_chain_tests archive_external_edit_tests archive_external_security_tests
    ui_navigation_tests ui_archive_states_tests flat_view_tests settings_tests properties_tests
    checksum_tests file_viewer_tests nested_archive_tests native_navigation_window_tests panel_sort_tests combine_name_tests panel_key_tests main_shell_tests panel_shell_tests volume_workflow_tests)
    if(TARGET ${target})
        target_link_libraries(${target} PRIVATE sevenzip_panel)
        target_link_libraries(${target} PRIVATE sevenzip_filesystem)
    endif()
endforeach()

if(BUILD_TESTING)
    get_target_property(source_properties_sources archive_opening_tests SOURCES)
    list(REMOVE_ITEM source_properties_sources tests/archive_opening_tests.cpp)
    get_target_property(source_properties_libraries archive_opening_tests LINK_LIBRARIES)
    qt_add_executable(source_properties_tests tests/source_properties_tests.cpp
        tests/layout_inspection.cpp src/propertiesdialog.cpp ${source_properties_sources})
    target_include_directories(source_properties_tests PRIVATE src tests)
    target_link_libraries(source_properties_tests PRIVATE ${source_properties_libraries})
    add_test(NAME source_properties COMMAND source_properties_tests -silent)
    set_tests_properties(source_properties PROPERTIES TIMEOUT 90
        RESOURCE_LOCK sevenzip_ui_artifacts ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/source_properties.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
endif()

if(BUILD_TESTING)
    qt_add_executable(language_tests tests/language_tests.cpp tests/layout_inspection.cpp)
    get_target_property(language_backend_sources archive_backend_tests SOURCES)
    list(REMOVE_ITEM language_backend_sources tests/archive_backend_tests.cpp)
    get_target_property(language_backend_libraries archive_backend_tests LINK_LIBRARIES)
    qt_add_executable(language_source_update_tests tests/language_source_update_tests.cpp
        ${language_backend_sources})
    target_include_directories(language_source_update_tests PRIVATE src)
    target_link_libraries(language_source_update_tests PRIVATE ${language_backend_libraries})
    add_test(NAME language_source_update COMMAND language_source_update_tests -silent)
    set_tests_properties(language_source_update PROPERTIES TIMEOUT 60)
    target_include_directories(language_tests PRIVATE src tests)
    target_link_libraries(language_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME language COMMAND language_tests -silent)
    set_tests_properties(language PROPERTIES TIMEOUT 60 RESOURCE_LOCK sevenzip_ui_artifacts
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(menu_page_tests tests/menu_page_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(menu_page_tests PRIVATE src tests)
    target_link_libraries(menu_page_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME menu_page COMMAND menu_page_tests -silent)
    set_tests_properties(menu_page PROPERTIES TIMEOUT 60
        RESOURCE_LOCK sevenzip_ui_artifacts ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(options_dialog_tests tests/options_dialog_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(options_dialog_tests PRIVATE src tests)
    target_link_libraries(options_dialog_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME options_dialog COMMAND options_dialog_tests -silent)
    set_tests_properties(options_dialog PROPERTIES TIMEOUT 60
        RESOURCE_LOCK sevenzip_ui_artifacts ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    get_target_property(options_window_sources settings_tests SOURCES)
    list(REMOVE_ITEM options_window_sources tests/settings_tests.cpp)
    get_target_property(options_window_libraries settings_tests LINK_LIBRARIES)
    qt_add_executable(help_ui_tests tests/help_ui_tests.cpp ${options_window_sources})
    target_include_directories(help_ui_tests PRIVATE src tests)
    target_link_libraries(help_ui_tests PRIVATE ${options_window_libraries})
    add_test(NAME help_ui COMMAND help_ui_tests -silent)
    set_tests_properties(help_ui PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/help_ui.ini")
    qt_add_executable(options_window_tests tests/options_window_tests.cpp ${options_window_sources})
    target_include_directories(options_window_tests PRIVATE src tests)
    target_link_libraries(options_window_tests PRIVATE ${options_window_libraries})
    add_test(NAME options_window COMMAND options_window_tests -silent)
    set_tests_properties(options_window PROPERTIES TIMEOUT 60 RESOURCE_LOCK sevenzip_ui_artifacts
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/options_window.ini")
    qt_add_executable(language_ui_tests tests/language_ui_tests.cpp
        tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(language_ui_tests PRIVATE src tests)
    target_link_libraries(language_ui_tests PRIVATE ${options_window_libraries})
    add_test(NAME language_ui COMMAND language_ui_tests -silent)
    set_tests_properties(language_ui PROPERTIES TIMEOUT 60 RESOURCE_LOCK sevenzip_ui_artifacts
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/language_ui.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(language_main_dialog_tests tests/language_main_dialog_tests.cpp
        tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(language_main_dialog_tests PRIVATE src tests)
    target_link_libraries(language_main_dialog_tests PRIVATE ${options_window_libraries})
    add_test(NAME language_main_dialog COMMAND language_main_dialog_tests -silent)
    set_tests_properties(language_main_dialog PROPERTIES TIMEOUT 60 RESOURCE_LOCK sevenzip_ui_artifacts
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/language_main_dialog.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(language_archive_command_tests tests/language_archive_command_tests.cpp
        tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(language_archive_command_tests PRIVATE src tests)
    target_link_libraries(language_archive_command_tests PRIVATE ${options_window_libraries})
    add_test(NAME language_archive_command COMMAND language_archive_command_tests -silent)
    set_tests_properties(language_archive_command PROPERTIES TIMEOUT 60 RESOURCE_LOCK sevenzip_ui_artifacts
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/language_archive_command.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    get_target_property(language_focus_sources compression_tests SOURCES)
    list(REMOVE_ITEM language_focus_sources tests/compression_tests.cpp)
    get_target_property(language_focus_libraries compression_tests LINK_LIBRARIES)
    qt_add_executable(language_focus_tests tests/language_focus_tests.cpp ${language_focus_sources})
    target_include_directories(language_focus_tests PRIVATE src tests)
    target_link_libraries(language_focus_tests PRIVATE ${language_focus_libraries})
    add_test(NAME language_focus COMMAND language_focus_tests -silent)
    set_tests_properties(language_focus PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/language_focus.ini")
endif()

# Every current window consumer uses the same language presentation adapter.
if(BUILD_TESTING)
    get_target_property(codec_catalog_backend_sources archive_backend_tests SOURCES)
    list(REMOVE_ITEM codec_catalog_backend_sources tests/archive_backend_tests.cpp)
    get_target_property(codec_catalog_backend_libraries archive_backend_tests LINK_LIBRARIES)
    qt_add_executable(codec_catalog_tests tests/codec_catalog_tests.cpp ${codec_catalog_backend_sources})
    target_include_directories(codec_catalog_tests PRIVATE src)
    target_link_libraries(codec_catalog_tests PRIVATE ${codec_catalog_backend_libraries} sevenzip_location)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(codec_catalog_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
        target_compile_definitions(compression_path_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
        target_compile_definitions(codec_creation_ui_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME codec_catalog COMMAND codec_catalog_tests -silent)
    qt_add_executable(native_benchmark_tests tests/native_benchmark_tests.cpp ${codec_catalog_backend_sources})
    target_include_directories(native_benchmark_tests PRIVATE src)
    target_link_libraries(native_benchmark_tests PRIVATE ${codec_catalog_backend_libraries} sevenzip_location sevenzip_panel)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(native_benchmark_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME native_benchmark COMMAND native_benchmark_tests -silent)
    set_tests_properties(native_benchmark PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(codec_creation_tests tests/codec_creation_tests.cpp ${codec_catalog_backend_sources})
    target_include_directories(codec_creation_tests PRIVATE src tests)
    target_link_libraries(codec_creation_tests PRIVATE ${codec_catalog_backend_libraries})
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(codec_creation_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME codec_creation COMMAND codec_creation_tests -silent)
    get_target_property(codec_update_sources source_update_backend_tests SOURCES)
    list(REMOVE_ITEM codec_update_sources tests/source_update_backend_tests.cpp)
    qt_add_executable(codec_update_tests tests/codec_update_tests.cpp ${codec_update_sources})
    target_include_directories(codec_update_tests PRIVATE src tests)
    target_link_libraries(codec_update_tests PRIVATE sevenzip_location Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(codec_update_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME codec_update COMMAND codec_update_tests -silent)
    qt_add_executable(codec_commands_ui_tests tests/codec_commands_ui_tests.cpp tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(codec_commands_ui_tests PRIVATE src tests)
    target_link_libraries(codec_commands_ui_tests PRIVATE ${options_window_libraries})
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(codec_commands_ui_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME codec_commands_ui COMMAND codec_commands_ui_tests -silent)
    set_tests_properties(codec_commands_ui PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/codec_commands_ui.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(native_zip_writer_tests tests/native_zip_writer_tests.cpp)
    qt_add_executable(native_comment_tests tests/native_comment_tests.cpp ${codec_update_sources})
    qt_add_executable(native_rar_memory_tests tests/native_rar_memory_tests.cpp tests/layout_inspection.cpp ${codec_update_sources})
    target_include_directories(native_rar_memory_tests PRIVATE src tests)
    target_link_libraries(native_rar_memory_tests PRIVATE sevenzip_location Qt6::Test)
    target_compile_definitions(native_rar_memory_tests PRIVATE
        "RAR_COMPRESSED_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/rar/test_read_format_rar5_compressed.rar\"")
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(native_rar_memory_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME native_rar_memory COMMAND native_rar_memory_tests -silent)
    set_tests_properties(native_rar_memory PROPERTIES TIMEOUT 45 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(native_rar_ui_tests tests/native_rar_ui_tests.cpp ${options_window_sources})
    qt_add_executable(native_hash_ui_tests tests/native_hash_ui_tests.cpp ${options_window_sources})
    qt_add_executable(benchmark_command_tests tests/benchmark_command_tests.cpp ${options_window_sources})
    target_include_directories(benchmark_command_tests PRIVATE src tests)
    target_link_libraries(benchmark_command_tests PRIVATE ${options_window_libraries})
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(benchmark_command_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME benchmark_command COMMAND benchmark_command_tests -silent)
    set_tests_properties(benchmark_command PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/benchmark_command.ini")
    target_include_directories(native_hash_ui_tests PRIVATE src tests)
    target_link_libraries(native_hash_ui_tests PRIVATE ${options_window_libraries})
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(native_hash_ui_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME native_hash_ui COMMAND native_hash_ui_tests -silent)
    set_tests_properties(native_hash_ui PROPERTIES TIMEOUT 45 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native_hash_ui.ini")
    target_include_directories(native_rar_ui_tests PRIVATE src tests)
    target_link_libraries(native_rar_ui_tests PRIVATE ${options_window_libraries})
    target_compile_definitions(native_rar_ui_tests PRIVATE
        "RAR_COMPRESSED_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/rar/test_read_format_rar5_compressed.rar\"")
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(native_rar_ui_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME native_rar_ui COMMAND native_rar_ui_tests -silent)
    set_tests_properties(native_rar_ui PROPERTIES TIMEOUT 45 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native_rar_ui.ini")
    qt_add_executable(native_comment_ui_tests tests/native_comment_ui_tests.cpp tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(native_comment_ui_tests PRIVATE src tests)
    target_link_libraries(native_comment_ui_tests PRIVATE ${options_window_libraries})
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(native_comment_ui_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME native_comment_ui COMMAND native_comment_ui_tests -silent)
    set_tests_properties(native_comment_ui PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native_comment_ui.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    target_include_directories(native_comment_tests PRIVATE src tests)
    target_link_libraries(native_comment_tests PRIVATE sevenzip_location Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(native_comment_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME native_comment COMMAND native_comment_tests -silent)
    set_tests_properties(native_comment PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    target_include_directories(native_zip_writer_tests PRIVATE src)
    target_link_libraries(native_zip_writer_tests PRIVATE sevenzip_zip sevenzip_location Qt6::Test)
    add_test(NAME native_zip_writer COMMAND native_zip_writer_tests -silent)
    set_tests_properties(native_zip_writer PROPERTIES TIMEOUT 30)
    qt_add_executable(native_folder_opening_tests tests/native_folder_opening_tests.cpp ${codec_update_sources})
    target_include_directories(native_folder_opening_tests PRIVATE src tests)
    target_link_libraries(native_folder_opening_tests PRIVATE sevenzip_location Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(native_folder_opening_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME native_folder_opening COMMAND native_folder_opening_tests -silent)
    set_tests_properties(native_folder_opening PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(native_archive_opening_tests tests/native_archive_opening_tests.cpp)
    target_include_directories(native_archive_opening_tests PRIVATE src)
    target_link_libraries(native_archive_opening_tests PRIVATE sevenzip_panel Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(native_archive_opening_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME native_archive_opening COMMAND native_archive_opening_tests -silent)
    set_tests_properties(native_archive_opening PROPERTIES TIMEOUT 30)
    set_tests_properties(codec_update PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    set_tests_properties(codec_creation PROPERTIES TIMEOUT 60)
    qt_add_executable(codec_probe_fixture tests/fixtures/codec_probe_fixture.cpp)
    target_link_libraries(codec_probe_fixture PRIVATE Qt6::Core)
    target_compile_definitions(codec_catalog_tests PRIVATE
        "CODEC_PROBE_FIXTURE=\"$<TARGET_FILE:codec_probe_fixture>\"")
    add_dependencies(codec_catalog_tests codec_probe_fixture)
    qt_add_executable(codec_probe_tests tests/codec_probe_tests.cpp)
    target_include_directories(codec_probe_tests PRIVATE src)
    target_link_libraries(codec_probe_tests PRIVATE sevenzip_filesystem Qt6::Test)
    target_compile_definitions(codec_probe_tests PRIVATE
        "CODEC_PROBE_FIXTURE=\"$<TARGET_FILE:codec_probe_fixture>\"")
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(codec_probe_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_dependencies(codec_probe_tests codec_probe_fixture)
    add_test(NAME codec_probe COMMAND codec_probe_tests -silent)
    set_tests_properties(codec_probe PROPERTIES TIMEOUT 20)
endif()

get_property(codec_backend_consumers DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(codec_backend_consumer IN LISTS codec_backend_consumers)
    get_target_property(codec_backend_sources ${codec_backend_consumer} SOURCES)
    if("src/archivebackend.cpp" IN_LIST codec_backend_sources)
        target_sources(${codec_backend_consumer} PRIVATE src/archivebackend_codecs.cpp)
    endif()
endforeach()

get_property(language_consumers DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(language_consumer IN LISTS language_consumers)
    get_target_property(language_consumer_sources ${language_consumer} SOURCES)
    if("src/mainwindow.cpp" IN_LIST language_consumer_sources)
        target_sources(${language_consumer} PRIVATE src/mainwindow_language.cpp)
    endif()
endforeach()

# Apply startup isolation after registering every consumer, including Language.
# Parallel CTest runs share these generated artifact directories. Keep their
# writers separate while allowing independent tests to run concurrently.
get_property(sevenzip_registered_tests DIRECTORY PROPERTY TESTS)
foreach(sevenzip_test IN LISTS sevenzip_registered_tests)
    set_property(TEST ${sevenzip_test} APPEND PROPERTY ENVIRONMENT
        "SEVENZIP_OWNED_TEMP_REGISTRY=${CMAKE_BINARY_DIR}/test-owned-temp/${sevenzip_test}")
    # MainWindow opens the current directory before a test navigates to its
    # fixture. Keep that startup input separate from hundreds of build outputs.
    # Each suite owns its directory so parallel runs cannot populate each other.
    if(TARGET ${sevenzip_test}_tests)
        get_target_property(sevenzip_test_sources ${sevenzip_test}_tests SOURCES)
        if("src/mainwindow.cpp" IN_LIST sevenzip_test_sources)
            set(sevenzip_test_startup_dir "${CMAKE_BINARY_DIR}/test-startup/${sevenzip_test}")
            file(MAKE_DIRECTORY "${sevenzip_test_startup_dir}")
            set_tests_properties(${sevenzip_test} PROPERTIES
                WORKING_DIRECTORY "${sevenzip_test_startup_dir}")
            # Saved panel paths outlive a QTemporaryDir fixture. Upstream
            # navigation then opens its parent, which must also be test-owned.
            set_property(TEST ${sevenzip_test} APPEND PROPERTY ENVIRONMENT
                "TMPDIR=${sevenzip_test_startup_dir}"
                "TMP=${sevenzip_test_startup_dir}"
                "TEMP=${sevenzip_test_startup_dir}")
        endif()
    endif()
    get_property(sevenzip_test_environment TEST ${sevenzip_test} PROPERTY ENVIRONMENT)
    if(sevenzip_test_environment MATCHES "SEVENZIP_UI_REPORT_DIR|SEVENZIP_SCREENSHOT_DIR")
        set_property(TEST ${sevenzip_test} APPEND PROPERTY RESOURCE_LOCK sevenzip_ui_artifacts)
    endif()
endforeach()
