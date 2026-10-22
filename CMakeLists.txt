cmake_minimum_required(VERSION 3.21)

project(sevenzip_desktop VERSION 0.1.5 LANGUAGES C CXX)

set(sevenzip_version_header "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/C/7zVersion.h")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${sevenzip_version_header}")
file(STRINGS "${sevenzip_version_header}" sevenzip_version_line REGEX "^#define MY_VERSION_NUMBERS ")
if(NOT sevenzip_version_line MATCHES "\"([0-9]+\\.[0-9]+)\"")
    message(FATAL_ERROR "Cannot read the upstream 7-Zip version")
endif()
set(SEVENZIP_UPSTREAM_VERSION "${CMAKE_MATCH_1}")
set(SEVENZIP_DESKTOP_DISPLAY_VERSION "${PROJECT_VERSION} (7-Zip ${SEVENZIP_UPSTREAM_VERSION})")
set_property(SOURCE src/main.cpp src/CPP/7zip/UI/FileManager/AboutDialog.cpp
    APPEND PROPERTY COMPILE_DEFINITIONS
    SEVENZIP_DESKTOP_DISPLAY_VERSION="${SEVENZIP_DESKTOP_DISPLAY_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)

# Relocated source headers must also resolve from unchanged upstream headers
# spelling MyVector.h, ../Common/MyVector.h or ../../Common/MyVector.h.
include_directories(src/CPP src/CPP/Common src/CPP/Windows src/CPP/7zip/Common)
set_source_files_properties(src/CPP/7zip/Common/LimitedStreams.cpp PROPERTIES
    INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/Common")

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Threads REQUIRED)
include(cmake/Exfat.cmake)
include(cmake/Cramfs.cmake)
include(cmake/Fat.cmake)
include(cmake/Ext.cmake)
include(cmake/Squashfs.cmake)
include(cmake/Ntfs.cmake)
include(cmake/Hfs.cmake)
include(cmake/Iso.cmake)
include(cmake/Udf.cmake)
include(cmake/Mbr.cmake)
include(cmake/Erofs.cmake)
include(cmake/LinuxFrames.cmake)
include(cmake/Brotli.cmake)
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
set(codec_icon_resource "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/Bundles/Format7zF/resource.rc")
set(resource_strings_header "${CMAKE_CURRENT_BINARY_DIR}/ResourceStrings.h")
add_custom_command(OUTPUT "${resource_strings_header}"
    COMMAND "${CMAKE_COMMAND}" "-DRESOURCE_FILES=${property_name_resource};${extract_resource};${file_manager_resource};${progress_resource};${update_resource};${overwrite_resource};${gui_resource};${explorer_resource};${language_resource};${codec_icon_resource}"
        "-DMIXED_RESOURCE_FILES=${file_manager_resource};${progress_resource};${overwrite_resource};${explorer_resource};${language_resource};${codec_icon_resource}"
        "-DOUTPUT_FILE=${resource_strings_header}" -P "${CMAKE_CURRENT_SOURCE_DIR}/scripts/lib/rc_strings.cmake"
    DEPENDS "${property_name_resource}" "${extract_resource}" "${file_manager_resource}"
        "${progress_resource}" "${update_resource}" "${overwrite_resource}" "${gui_resource}" "${explorer_resource}" "${language_resource}" "${codec_icon_resource}" scripts/lib/rc_strings.cmake
    VERBATIM)

qt_standard_project_setup()

if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 17)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    add_library(sevenzip_device_access STATIC
        src/deviceaccess.h src/deviceaccess_mac.h src/deviceaccess_mac.mm
        src/deviceaccess_posix.cpp src/deviceaccess_posix.h
        src/deviceauthclient_mac.cpp src/deviceauthsocket_mac.h)
    target_include_directories(sevenzip_device_access PUBLIC src third_party/7zip/CPP)
    target_link_libraries(sevenzip_device_access PUBLIC Qt6::Core
        "-framework DiskArbitration" "-framework IOKit" "-framework Foundation")
    set_target_properties(sevenzip_device_access PROPERTIES AUTOMOC OFF AUTOUIC OFF)
    add_executable(sevenzip_device_reader src/devicereader_mac.cpp)
    target_link_libraries(sevenzip_device_reader PRIVATE sevenzip_device_access)
    set_target_properties(sevenzip_device_reader PROPERTIES
        OUTPUT_NAME SevenzipDeviceReader AUTOMOC OFF AUTOUIC OFF)
elseif(WIN32)
    add_library(sevenzip_device_access STATIC
        src/deviceaccess.h src/deviceaccess_win.h src/deviceaccess_win.cpp
        src/devicehandle_win.cpp src/deviceauthclient_win.cpp src/devicepipe_win.cpp src/devicepipe_win.h)
    target_include_directories(sevenzip_device_access PUBLIC src third_party/7zip/CPP)
    target_link_libraries(sevenzip_device_access PUBLIC Qt6::Core setupapi cfgmgr32 advapi32 ole32 shell32)
    set_target_properties(sevenzip_device_access PROPERTIES AUTOMOC OFF AUTOUIC OFF)
    qt_add_executable(sevenzip_device_reader WIN32 src/devicereader_win.cpp)
    target_link_libraries(sevenzip_device_reader PRIVATE sevenzip_device_access)
    set_target_properties(sevenzip_device_reader PROPERTIES
        OUTPUT_NAME SevenzipDeviceReader AUTOMOC OFF AUTOUIC OFF)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_library(sevenzip_device_access STATIC
        src/deviceaccess.h src/deviceaccess_linux.cpp
        src/deviceaccess_posix.cpp src/deviceaccess_posix.h)
    target_include_directories(sevenzip_device_access PUBLIC src third_party/7zip/CPP)
    target_link_libraries(sevenzip_device_access PUBLIC Qt6::Core Qt6::DBus)
    set_target_properties(sevenzip_device_access PROPERTIES AUTOMOC OFF AUTOUIC OFF)
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
    src/qtdiagnostictext.cpp src/qtdiagnostictext.h
    src/qttranslationcatalog.cpp src/qttranslationcatalog.h
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
    third_party/7zip/C/Threads.c src/CPP/Windows/ErrorMsg.cpp
    src/CPP/Windows/DLL.cpp src/CPP/Windows/DLL.h
    third_party/7zip/CPP/Common/StringToInt.cpp
    third_party/7zip/CPP/Common/MyString.cpp third_party/7zip/CPP/Common/IntToString.cpp
    third_party/7zip/CPP/Common/MyWindows.cpp
    src/qtownedtempidentity.cpp src/qtownedtempidentity.h
    src/qtownedtempregistry.cpp src/qtownedtempregistry.h
    src/CPP/Windows/FileDir.cpp src/CPP/Windows/FileDir.h
    src/CPP/Windows/FileName.cpp
    third_party/7zip/CPP/Windows/FileFind.cpp
    src/CPP/Windows/FileIO.cpp
    third_party/7zip/CPP/Windows/TimeUtils.cpp
    third_party/7zip/CPP/7zip/Common/FileStreams.cpp
    third_party/7zip/CPP/7zip/Common/StreamUtils.cpp
    third_party/7zip/CPP/7zip/Common/StreamObjects.cpp
    src/CPP/7zip/Common/LimitedStreams.cpp
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
    src/CPP/7zip/UI/FileManager/RegistryUtils.cpp src/CPP/7zip/UI/FileManager/RegistryUtils.h
    third_party/7zip/CPP/7zip/UI/FileManager/StringUtils.cpp)
set_target_properties(sevenzip_common PROPERTIES AUTOMOC OFF AUTOUIC OFF)
target_include_directories(sevenzip_common PUBLIC src/CPP third_party/7zip/CPP PRIVATE src)
target_include_directories(sevenzip_common PRIVATE third_party/7zip/CPP/Windows)
target_link_libraries(sevenzip_common PUBLIC Qt6::Core)
target_compile_definitions(sevenzip_common PUBLIC Z7_EXTERNAL_CODECS)
if(WIN32)
    target_compile_definitions(sevenzip_common PUBLIC Z7_DEVICE_FILE)
endif()
target_compile_definitions(sevenzip_hash_registry PRIVATE Z7_EXTERNAL_CODECS)
target_link_libraries(sevenzip_common PUBLIC sevenzip_hash_registration)
target_link_libraries(sevenzip_common PUBLIC ${CMAKE_DL_LIBS})
target_link_libraries(sevenzip_common PUBLIC Threads::Threads)
target_sources(sevenzip_common PRIVATE
    src/CPP/Common/CommandLineParser.cpp
    third_party/7zip/CPP/Common/ListFileUtils.cpp
    src/CPP/7zip/UI/Common/ArchiveCommandLine.cpp)
target_include_directories(sevenzip_common PRIVATE
    src/CPP/Common src/CPP/7zip/UI/Common)
if(WIN32)
    target_sources(sevenzip_common PRIVATE third_party/7zip/C/DllSecur.c)
    target_sources(sevenzip_common PRIVATE third_party/7zip/CPP/Windows/FileSystem.cpp
        third_party/7zip/CPP/Windows/FileSystem.h)
    target_sources(sevenzip_common PRIVATE third_party/7zip/CPP/Common/Random.cpp)
    set_source_files_properties(third_party/7zip/CPP/Common/ListFileUtils.cpp
        src/CPP/7zip/UI/Common/ArchiveCommandLine.cpp PROPERTIES
        COMPILE_DEFINITIONS "_UNICODE;UNICODE")
    set_source_files_properties(src/CPP/7zip/UI/Common/CompressCall.cpp PROPERTIES
        COMPILE_DEFINITIONS "_UNICODE;UNICODE")
    target_sources(sevenzip_common PRIVATE src/CPP/Windows/Shell.cpp
        third_party/7zip/CPP/Windows/MemoryGlobal.cpp)
    set_source_files_properties(src/CPP/Windows/Shell.cpp
        src/CPP/7zip/UI/FileManager/BrowseDialog2.cpp
        third_party/7zip/CPP/Windows/MemoryGlobal.cpp PROPERTIES
        COMPILE_DEFINITIONS "_UNICODE;UNICODE")
    target_link_libraries(sevenzip_common PUBLIC shell32)
    target_sources(sevenzip_common PRIVATE third_party/7zip/CPP/Windows/Registry.cpp)
    target_sources(sevenzip_common PRIVATE third_party/7zip/CPP/Windows/Menu.cpp)
    set_source_files_properties(third_party/7zip/CPP/Windows/Menu.cpp PROPERTIES
        COMPILE_DEFINITIONS "_UNICODE;UNICODE")
    target_link_libraries(sevenzip_common PUBLIC user32)
    target_sources(sevenzip_common PRIVATE third_party/7zip/CPP/Windows/ProcessUtils.cpp)
    set_source_files_properties(third_party/7zip/CPP/Windows/ProcessUtils.cpp
        third_party/7zip/CPP/Windows/Registry.cpp src/CPP/7zip/UI/Common/LoadCodecs.cpp PROPERTIES
        COMPILE_DEFINITIONS "_UNICODE;UNICODE")
    target_sources(sevenzip_common PRIVATE third_party/7zip/CPP/Windows/SecurityUtils.cpp)
    target_sources(sevenzip_common PRIVATE third_party/7zip/CPP/Windows/MemoryLock.cpp)
    set_source_files_properties(src/qtnativemenupageservices.cpp PROPERTIES
        COMPILE_DEFINITIONS "_UNICODE;UNICODE")
    set_source_files_properties(third_party/7zip/CPP/Windows/MemoryLock.cpp
        src/CPP/7zip/UI/FileManager/FM.cpp src/qtmemorylockservices.cpp
        PROPERTIES COMPILE_DEFINITIONS "_UNICODE;UNICODE")
    target_link_libraries(sevenzip_common PUBLIC ole32 oleaut32 uuid advapi32)
endif()

include(cmake/native_zip.cmake)
include(cmake/native_rar.cmake)

add_library(sevenzip_progress_sync STATIC
    src/CPP/7zip/UI/FileManager/ProgressDialog2.cpp
    src/qtworkerprogress.cpp src/qtworkerprogress.h)
target_compile_definitions(sevenzip_progress_sync PRIVATE SEVENZIP_PROGRESS_SYNC_ONLY)
target_include_directories(sevenzip_progress_sync PUBLIC src)
target_link_libraries(sevenzip_progress_sync PUBLIC Qt6::Core sevenzip_common)

add_library(sevenzip_system_icons STATIC
    src/CPP/7zip/UI/FileManager/SysIconUtils.cpp src/CPP/7zip/UI/FileManager/SysIconUtils.h)
set_target_properties(sevenzip_system_icons PROPERTIES AUTOMOC OFF AUTOUIC OFF)
target_link_libraries(sevenzip_system_icons PUBLIC Qt6::Widgets sevenzip_common)
if(WIN32)
    target_link_libraries(sevenzip_system_icons PUBLIC shell32 comctl32)
endif()

add_library(sevenzip_name_sort STATIC src/CPP/7zip/UI/FileManager/PanelSort.cpp)
set_target_properties(sevenzip_name_sort PROPERTIES AUTOMOC OFF AUTOUIC OFF)
target_compile_definitions(sevenzip_name_sort PRIVATE SEVENZIP_NAME_SORT_ONLY)
target_include_directories(sevenzip_name_sort PUBLIC src)
target_link_libraries(sevenzip_name_sort PUBLIC Qt6::Core sevenzip_common)

# The private API dependency is intentional; require the matching Qt build below.
set(QT_NO_PRIVATE_MODULE_WARNING ON)
if(NOT TARGET Qt6::GuiPrivate)
    find_package(Qt6GuiPrivate ${Qt6Gui_VERSION} EXACT QUIET CONFIG
        HINTS "${Qt6Gui_DIR}/../Qt6GuiPrivate")
endif()
if(NOT TARGET Qt6::GuiPrivate)
    message(FATAL_ERROR "Qt GUI private development headers are required for native keyboard decoding")
endif()
add_library(sevenzip_native_keys STATIC src/qtnativepanelkeys.cpp src/qtnativepanelkeys.h)
target_include_directories(sevenzip_native_keys PUBLIC src)
target_link_libraries(sevenzip_native_keys PRIVATE Qt6::GuiPrivate)
if(APPLE)
    target_link_libraries(sevenzip_native_keys PRIVATE "-framework Carbon")
endif()
set_target_properties(sevenzip_native_keys PROPERTIES AUTOMOC OFF AUTOUIC OFF)

add_library(sevenzip_panel STATIC src/CPP/7zip/UI/FileManager/App.cpp
    src/qtsemanticcolors.cpp src/qtsemanticcolors.h
    src/qtmemorylockservices.cpp src/qtmemorylockservices.h
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
    src/qtdialogscreenservices.cpp src/qtdialogscreenservices.h
    src/CPP/Windows/CommonDialog.cpp src/CPP/Windows/CommonDialog.h
    src/CPP/7zip/UI/FileManager/MyLoadMenu.cpp src/CPP/7zip/UI/FileManager/MyLoadMenu.h
    src/qtfilemenuservices.h src/qtfilemanagercommandservices.h
    src/CPP/7zip/UI/FileManager/FM.cpp src/qtfilemanagerwindowservices.h src/qtfilemanagerstartup.h
    src/qtappoperationstate.cpp src/qtappoperationstate.h
    src/CPP/7zip/UI/FileManager/PanelDrag.cpp
    src/CPP/7zip/UI/GUI/HashGUI.cpp src/CPP/7zip/UI/GUI/HashGUI.h
    src/CPP/7zip/UI/GUI/BenchmarkDialog.cpp src/CPP/7zip/UI/GUI/BenchmarkDialog.h
    src/CPP/7zip/UI/GUI/CompressDialog.cpp
    src/CPP/7zip/UI/Common/CompressCall.cpp
    src/CPP/7zip/UI/Common/CompressCall2.cpp src/CPP/7zip/UI/Common/CompressCall.h
    src/qtcompresscallservices.h
    src/qtupdateguiservices.h
    src/qtbenchmarkcontrols.cpp src/qtbenchmarkcontrols.h
    src/qtbenchmarkmetrics.cpp
    src/CPP/7zip/UI/FileManager/LinkDialog.cpp src/CPP/7zip/UI/FileManager/LinkDialog.h
    src/qtlinkcontrols.cpp src/qtlinkcontrols.h
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
    src/CPP/7zip/UI/Explorer/ContextMenu.h
    src/CPP/7zip/UI/Explorer/RegistryContextMenu.cpp src/CPP/7zip/UI/Explorer/RegistryContextMenu.h
    src/qtshellregistryservices.h
    src/qtmenupageservices.h
    src/qtnativemenupageservices.cpp src/qtnativemenupageservices.h
    src/qtshellmenupresentation.cpp src/qtshellmenupresentation.h src/qtcontextmenuservices.h
    src/CPP/7zip/UI/FileManager/RootFolder.cpp src/CPP/7zip/UI/FileManager/RootFolder.h
    src/CPP/7zip/UI/FileManager/PanelSort.cpp src/CPP/7zip/UI/FileManager/PanelSort.h
    src/archiveprogressoperation.cpp src/archiveprogressoperation.h
    src/archiveprogresstransport.cpp src/archiveprogresstransport.h src/qtprogresstransport.h
    src/qtcallbackprogresstransport.cpp src/qtcallbackprogresstransport.h
    src/CPP/7zip/UI/FileManager/FormatUtils.cpp src/CPP/7zip/UI/FileManager/FormatUtils.h
    src/CPP/7zip/UI/FileManager/ProgressDialog2.cpp src/CPP/7zip/UI/FileManager/ProgressDialog2.h
    src/CPP/7zip/UI/FileManager/MessagesDialog.cpp src/CPP/7zip/UI/FileManager/MessagesDialog.h
    src/CPP/7zip/UI/FileManager/MessagesDialogRes.h
    src/CPP/7zip/UI/GUI/UpdateCallbackGUI2.cpp src/CPP/7zip/UI/GUI/UpdateCallbackGUI2.h
    src/CPP/7zip/UI/GUI/UpdateCallbackGUI.cpp src/CPP/7zip/UI/GUI/UpdateCallbackGUI.h
    src/CPP/7zip/UI/GUI/ExtractGUI.cpp src/CPP/7zip/UI/GUI/ExtractGUI.h
    src/CPP/7zip/UI/GUI/ExtractDialog.cpp src/CPP/7zip/UI/GUI/ExtractDialog.h
    src/CPP/7zip/UI/FileManager/UpdateCallback100.cpp src/CPP/7zip/UI/FileManager/UpdateCallback100.h
    src/qtcallbackpassword.cpp src/qtcallbackpassword.h
    src/qtprogresscontrols.cpp src/qtprogresscontrols.h
    src/qtdialogunits.cpp src/qtdialogunits.h
    src/qtdialoglayout.cpp src/qtdialoglayout.h
    src/qtdisclosure.cpp src/qtdisclosure.h
    src/qtpanellist.cpp src/qtpanellist.h
    src/qtpanelfolder.cpp src/qtpanelfolderraw.cpp src/qtpanelfolder.h
    src/CPP/7zip/UI/FileManager/App.h src/CPP/7zip/UI/FileManager/Panel.cpp
    src/CPP/7zip/UI/FileManager/SettingsPage.cpp src/CPP/7zip/UI/FileManager/SettingsPage.h
    src/qtmemorylimitspin.cpp src/qtmemorylimitspin.h
    src/CPP/7zip/UI/FileManager/FoldersPage.cpp src/CPP/7zip/UI/FileManager/FoldersPage.h
    src/CPP/7zip/UI/FileManager/PanelFolderChange.cpp
    src/CPP/7zip/UI/FileManager/PanelMenu.cpp
    src/CPP/7zip/UI/FileManager/PropertyName.cpp src/CPP/7zip/UI/FileManager/PropertyName.h
    src/qtlangcontrols.cpp src/qtlangcontrols.h
    src/qtcommandicons.cpp src/qtcommandicons.h
    src/qtmainwindowchrome.cpp src/qtmainwindowchrome.h
    src/qtpaneladdresscombo.cpp src/qtpaneladdresscombo.h
    src/qtlangbuttonbox.cpp src/qtlangbuttonbox.h
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
target_compile_definitions(sevenzip_panel PRIVATE SEVENZIP_PROGRESS_PRESENTATION_ONLY)
target_link_libraries(sevenzip_panel PUBLIC sevenzip_progress_sync)
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
qt_add_resources(sevenzip_panel upstream_shell_icons
    PREFIX "/upstream/7zip/UI/Explorer"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/Explorer"
    FILES third_party/7zip/CPP/7zip/UI/Explorer/MenuLogo.bmp)
target_include_directories(sevenzip_panel PUBLIC src)
target_include_directories(sevenzip_panel PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(sevenzip_panel PUBLIC Qt6::Widgets sevenzip_common sevenzip_system_icons
    sevenzip_native_keys)
target_sources(sevenzip_panel PRIVATE src/CPP/7zip/UI/Explorer/MyMessages.cpp src/qtguimessages.h)
if(WIN32)
    target_sources(sevenzip_panel PRIVATE
        src/qtpaneldragnative.cpp src/qtpaneldragnative.h
        third_party/7zip/CPP/7zip/UI/FileManager/EnumFormatEtc.cpp
        src/CPP/7zip/UI/FileManager/AltStreamsFolder.cpp
        src/CPP/7zip/UI/FileManager/AltStreamsFolder.h
        src/wincontextmenuservices.cpp src/wincontextmenuservices.h
        src/winshellmenupresentation.cpp src/winshellmenupresentation.h)
    target_link_libraries(sevenzip_panel PUBLIC shell32)
endif()
target_sources(sevenzip_panel PRIVATE src/qtprogresswait.cpp src/qtprogresswait.h)
if(WIN32)
    add_library(sevenzip_shell SHARED src/CPP/7zip/UI/Explorer/DllExportsExplorer.cpp
        third_party/7zip/CPP/7zip/UI/Explorer/Explorer.def)
    set_target_properties(sevenzip_shell PROPERTIES OUTPUT_NAME "7-zip" AUTOUIC OFF AUTOMOC OFF)
    target_compile_definitions(sevenzip_shell PRIVATE _UNICODE UNICODE)
    target_link_libraries(sevenzip_shell PRIVATE sevenzip_panel)
    file(READ "${explorer_resource}" sevenzip_native_menu_resource)
    string(REPLACE "\"resource.h\""
        "\"${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/Explorer/resource.h\""
        sevenzip_native_menu_resource "${sevenzip_native_menu_resource}")
    string(REPLACE "\"../../UI/Explorer/MenuLogo.bmp\""
        "\"${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/Explorer/MenuLogo.bmp\""
        sevenzip_native_menu_resource "${sevenzip_native_menu_resource}")
    # Original Explorer resource.rc icon entry, with a build-only absolute path.
    string(APPEND sevenzip_native_menu_resource
        "\nIDI_ICON ICON \"${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/FileManager/FM.ico\"\n")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/native_context_menu.rc"
        "${sevenzip_native_menu_resource}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${explorer_resource}")
    target_sources(sevenzip_shell PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/native_context_menu.rc")
endif()
if(APPLE)
    target_sources(sevenzip_panel PRIVATE src/qttaskbarprogress_mac.mm src/qttaskbarprogress.h)
    target_link_libraries(sevenzip_panel PUBLIC "-framework AppKit")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(sevenzip_panel PUBLIC sevenzip_linux_taskbar)
endif()

add_library(sevenzip_filesystem STATIC
    src/archivesfxmodule.cpp src/archivesfxmodule.h
    src/qtarchivehashoperation.cpp src/qtarchivehashoperation.h
    src/qtarchivecopypreparation.cpp src/qtarchivecopypreparation.h
    src/archiveenginelocation.cpp src/archiveenginelocation.h
    src/qtownedtempdir.cpp src/qtownedtempdir.h
    src/CPP/7zip/UI/FileManager/LangUtils.cpp src/CPP/7zip/UI/FileManager/LangUtils.h
    src/qtlangdialogcontrols.h
    src/qtlanguageservices.cpp src/qtlanguageservices.h
    src/qtupstreamtranslations.h
    src/CPP/Windows/ResourceString.cpp src/CPP/Windows/ResourceString.h
    "${resource_strings_header}"
    src/CPP/7zip/UI/Common/WorkDir.cpp src/CPP/7zip/UI/Common/WorkDir.h
    src/CPP/7zip/UI/Common/Update.cpp src/CPP/7zip/UI/Common/Update.h
    src/CPP/7zip/UI/Common/TempFiles.cpp src/CPP/7zip/UI/Common/TempFiles.h
    src/CPP/7zip/UI/Common/UpdateCallback.cpp src/CPP/7zip/UI/Common/UpdateCallback.h
    src/CPP/7zip/UI/Common/UpdatePair.cpp
    third_party/7zip/CPP/7zip/UI/Common/UpdateProduce.cpp
    src/qtarchivedeleteinputs.cpp src/qtarchivedeleteinputs.h
    src/qtarchivecreationlifecycle.cpp src/qtarchivecreationlifecycle.h
    src/CPP/7zip/Common/MultiOutStream.cpp src/CPP/7zip/Common/MultiOutStream.h
    src/qtarchivevolumeoutput.cpp src/qtarchivevolumeoutput.h
    src/CPP/7zip/UI/Common/ArchiveExtractCallback.cpp src/CPP/7zip/UI/Common/ArchiveExtractCallback.h
    src/CPP/7zip/UI/Common/Extract.cpp src/CPP/7zip/UI/Common/Extract.h
    src/CPP/7zip/UI/Common/OpenArchive.cpp src/CPP/7zip/UI/Common/OpenArchive.h
    src/archivecodeccatalog.cpp src/archivecodeccatalog.h
    src/archivecodecimage.cpp src/archivecodecimage.h
    src/archivenativeopen.cpp src/archivenativeopen.h
    src/archivedevice.cpp src/archivedevice.h
    src/archivebackend_device.cpp
    src/deviceinput.cpp src/deviceaccess.h
    src/archivecodecprobe.cpp src/archivecodecprobe.h
    third_party/7zip/CPP/7zip/UI/Common/ArchiveOpenCallback.cpp
    third_party/7zip/CPP/7zip/UI/Common/SetProperties.cpp
    third_party/7zip/CPP/7zip/Common/FilePathAutoRename.cpp
    src/CPP/7zip/UI/FileManager/FSFolder.cpp
    src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp
    src/qtlinkservices.cpp src/qtlinkservices.h
    src/qtfiletransfercopycallback.cpp src/qtfiletransfercopycallback.h
    src/CPP/7zip/UI/FileManager/TextPairs.cpp
    src/CPP/7zip/UI/FileManager/FSDrives.cpp src/CPP/7zip/UI/FileManager/FSDrives.h
    src/qtvolumemounts.cpp src/qtvolumemounts.h
    third_party/7zip/CPP/7zip/UI/Common/PropIDUtils.cpp)
file(GLOB_RECURSE sevenzip_help_assets CONFIGURE_DEPENDS resources/7zip/Help/*.htm resources/7zip/Help/*.css)
qt_add_resources(sevenzip_panel upstream_help PREFIX "/upstream/7zip/Help"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources/7zip/Help" FILES ${sevenzip_help_assets})
target_compile_definitions(sevenzip_panel PRIVATE SEVENZIP_DESKTOP_VERSION="${PROJECT_VERSION}")
if(WIN32)
    target_sources(sevenzip_filesystem PRIVATE third_party/7zip/CPP/Windows/Net.cpp
        third_party/7zip/CPP/Windows/Net.h)
    target_link_libraries(sevenzip_filesystem PUBLIC mpr)
endif()
target_include_directories(sevenzip_filesystem PUBLIC src)
target_include_directories(sevenzip_filesystem PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(sevenzip_filesystem PRIVATE third_party/7zip/CPP/Windows)
target_link_libraries(sevenzip_filesystem PUBLIC Qt6::Core sevenzip_common sevenzip_zip sevenzip_progress_sync)
target_link_libraries(sevenzip_filesystem PUBLIC sevenzip_system_icons sevenzip_name_sort)
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
    src/qtassociationfileopen.cpp src/qtassociationfileopen.h
    src/CPP/7zip/UI/Agent/ArchiveFolderOpen.cpp
    src/CPP/7zip/UI/FileManager/LangPage.cpp src/CPP/7zip/UI/FileManager/LangPage.h
    src/CPP/7zip/UI/FileManager/EditPage.cpp src/CPP/7zip/UI/FileManager/EditPage.h
    src/CPP/7zip/UI/FileManager/MenuPage.cpp src/CPP/7zip/UI/FileManager/MenuPage.h
    src/CPP/7zip/UI/FileManager/SystemPage.cpp src/CPP/7zip/UI/FileManager/SystemPage.h
    src/CPP/7zip/UI/FileManager/FilePlugins.cpp src/CPP/7zip/UI/FileManager/FilePlugins.h
    src/CPP/7zip/UI/FileManager/RegistryAssociations.cpp src/CPP/7zip/UI/FileManager/RegistryAssociations.h
    src/qtassociationservices.cpp src/qtassociationservices.h src/qtassociationplatform.h
    src/CPP/7zip/UI/FileManager/OptionsDialog.cpp src/qtfmoptionsdialog.h
    src/qtfmoptionslayout.cpp src/qtsystempagepresentation.cpp
    src/qtarchivebatchopen.cpp src/qtarchivebatchopen.h
    src/qtarchivebatchextract.cpp src/qtarchivebatchextract.h src/qtextractpathpolicy.h
    src/qtshellarchiveservices.cpp src/qtshellarchiveservices.h
    src/qtarchivebackendrequest.cpp src/qtarchivebackendrequest.h
    src/qtarchivesourceupdate.cpp src/qtarchivesourceupdate.h
    src/qtarchivesourcerename.cpp src/qtarchivesourcerename.h
    src/qtarchivereopen.cpp src/qtarchivereopen.h
    src/qtarchivesourceextract.cpp
    src/CPP/7zip/UI/Agent/AgentOut.cpp
    src/CPP/7zip/UI/Agent/ArchiveFolderOut.cpp
    src/CPP/7zip/UI/Agent/ArchiveFolder.cpp
    src/CPP/7zip/UI/Agent/UpdateCallbackAgent.cpp src/CPP/7zip/UI/Agent/UpdateCallbackAgent.h
    src/qtarchiveproperties.cpp src/qtarchiveproperties.h
    src/qtarchiveitemstream.cpp src/qtarchiveitemstream.h
    src/qtarchivefoldermanager.cpp src/qtarchivefoldermanager.h
    src/qtarchiveagentopen.h
    src/qtarchivenativeopenprogress.cpp src/qtarchivenativeopenprogress.h
    src/qtarchivestreamopen.h
    src/qtarchivefileresource.cpp src/qtarchivefileresource.h
    src/qtarchivedevicefolder.cpp
    src/deviceselectiondialog.cpp src/deviceselectiondialog.h src/qtdevicecommands.cpp
    src/qtfilesystemreadprompt.cpp src/qtfilesystemreadprompt.h
    src/qtarchivenativeproperties.cpp
    src/qtfolderpropertycapabilities.h
    src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/AgentProxy.cpp)
target_include_directories(sevenzip_location PUBLIC src)
target_link_libraries(sevenzip_location PUBLIC sevenzip_filesystem sevenzip_panel)
target_link_libraries(sevenzip_location PRIVATE sevenzip_device_access)
target_sources(sevenzip_location PRIVATE src/CPP/7zip/UI/GUI/GUI.cpp src/qtguicommand.h)
target_include_directories(sevenzip_location PRIVATE src/CPP/Common src/CPP/7zip/UI/Common)
if(WIN32)
    set_source_files_properties(src/CPP/7zip/UI/GUI/GUI.cpp PROPERTIES
        COMPILE_DEFINITIONS "_UNICODE;UNICODE")
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-unix-2.0)
    target_sources(sevenzip_location PRIVATE src/qtassociationplatform_linux.cpp)
    target_link_libraries(sevenzip_location PRIVATE PkgConfig::GIO)
endif()
if(WIN32)
    # Source association records expose CSysString. Keep the Windows UTF-16
    # definition consistent between the library and its page/service consumers.
    target_compile_definitions(sevenzip_location PUBLIC _UNICODE UNICODE)
    target_sources(sevenzip_location PRIVATE src/qtassociationplatform_win.cpp)
elseif(APPLE)
    target_sources(sevenzip_location PRIVATE src/qtassociationplatform_mac.cpp src/qtassociationplatform_mac.mm
        src/qtmacassociationnative.h)
    target_link_libraries(sevenzip_location PRIVATE "-framework AppKit" "-framework UniformTypeIdentifiers")
endif()

add_library(sevenzip_folder_loader STATIC
    src/filesystemfolderloader.cpp src/filesystemfolderloader.h
    src/qtfolderprojection.cpp src/qtfolderprojection.h)
target_include_directories(sevenzip_folder_loader PUBLIC src)
target_link_libraries(sevenzip_folder_loader PUBLIC sevenzip_location Qt6::Core)

qt_add_executable(sevenzip_desktop
    src/main.cpp src/desktopidentity.h
      src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp  src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_rename.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp
    src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h
    src/combodialog.h
    src/copydialog.h
    src/extractdialog.h
    src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp
    src/archivebackend.h
    src/filesystemscanner.cpp
    src/filesystemscanner.h
    src/settingsdialog.h
    src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h
)

target_include_directories(sevenzip_desktop PRIVATE src)
target_link_libraries(sevenzip_desktop PRIVATE Qt6::Widgets)
target_sources(sevenzip_desktop PRIVATE src/qtguimain.cpp src/qtfilemanagermain.cpp)
if(WIN32)
    # Original resource.rc IDI_ICON entry, with build-local absolute paths.
    add_dependencies(sevenzip_desktop sevenzip_device_reader)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/application_icon.rc"
        "#include \"${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/FileManager/resourceGui.h\"\nIDI_ICON ICON \"${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/FileManager/FM.ico\"\n")
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/application_icon.rc" PROPERTIES
        OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/FileManager/FM.ico")
    target_sources(sevenzip_desktop PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/application_icon.rc")
    add_custom_command(TARGET sevenzip_desktop POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:sevenzip_desktop>
            "$<TARGET_FILE_DIR:sevenzip_desktop>/7zG.exe"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:sevenzip_desktop>
            "$<TARGET_FILE_DIR:sevenzip_desktop>/7zFM.exe"
        VERBATIM)
else()
    add_custom_command(TARGET sevenzip_desktop POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:sevenzip_desktop>
            "$<TARGET_FILE_DIR:sevenzip_desktop>/7zG"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:sevenzip_desktop>
            "$<TARGET_FILE_DIR:sevenzip_desktop>/7zFM"
        VERBATIM)
endif()

set_target_properties(sevenzip_desktop PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
sevenzip_deploy_exfat()
sevenzip_deploy_cramfs()
sevenzip_deploy_fat()
sevenzip_deploy_ext()
sevenzip_deploy_squashfs()
sevenzip_deploy_ntfs()
sevenzip_deploy_hfs()
sevenzip_deploy_iso()
sevenzip_deploy_udf()
sevenzip_deploy_mbr()
sevenzip_deploy_erofs()
sevenzip_deploy_linux_frames()
sevenzip_deploy_brotli()
if(APPLE)
    include(cmake/MacAssociationBundle.cmake)
endif()
if(WIN32)
    include(cmake/WindowsInstall.cmake)
else()
    include(cmake/UnixInstall.cmake)
endif()
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/PackageInfo.json"
    CONTENT "{\"version\":\"${PROJECT_VERSION}\",\"display_version\":\"${SEVENZIP_DESKTOP_DISPLAY_VERSION}\",\"upstream_version\":\"${SEVENZIP_UPSTREAM_VERSION}\",\"qt_version\":\"${Qt6_VERSION}\"}\n")

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_executable(object_vector_lifetime_tests tests/object_vector_lifetime_tests.cpp)
    target_include_directories(object_vector_lifetime_tests PRIVATE src/CPP third_party/7zip/CPP)
    set_target_properties(object_vector_lifetime_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF)
    add_test(NAME object_vector_lifetime COMMAND object_vector_lifetime_tests)
    set_tests_properties(object_vector_lifetime PROPERTIES TIMEOUT 15)
    qt_add_executable(drag_drop_tests tests/drag_drop_tests.cpp tests/panel_load_folder.cpp)
    target_link_libraries(drag_drop_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME drag_drop COMMAND drag_drop_tests -silent)
    set_tests_properties(drag_drop PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    qt_add_executable(drag_protocol_tests tests/drag_protocol_tests.cpp)
    target_link_libraries(drag_protocol_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME drag_protocol COMMAND drag_protocol_tests -silent)
    set_tests_properties(drag_protocol PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    if(WIN32)
        qt_add_executable(native_drag_protocol_tests tests/native_drag_protocol_tests.cpp)
        target_link_libraries(native_drag_protocol_tests PRIVATE sevenzip_panel Qt6::Test)
        add_test(NAME native_drag_protocol COMMAND native_drag_protocol_tests -silent)
        set_tests_properties(native_drag_protocol PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
        qt_add_executable(native_drag_qt_tests tests/native_drag_qt_tests.cpp)
        target_link_libraries(native_drag_qt_tests PRIVATE sevenzip_panel Qt6::Test)
        add_test(NAME native_drag_qt COMMAND native_drag_qt_tests -silent)
        set_tests_properties(native_drag_qt PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=windows" TIMEOUT 30)
    endif()
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
    qt_add_executable(temp_detached_launch_tests tests/temp_detached_launch_tests.cpp)
    target_link_libraries(temp_detached_launch_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME temp_detached_launch COMMAND temp_detached_launch_tests -silent)
    set_tests_properties(temp_detached_launch PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/temp-detached-settings.ini"
        TIMEOUT 30)
    set_tests_properties(owned_temp PROPERTIES TIMEOUT 30)
    qt_add_executable(help_source_tests tests/help_source_tests.cpp tests/layout_inspection.cpp)
    target_compile_definitions(help_source_tests PRIVATE SEVENZIP_DESKTOP_VERSION="${PROJECT_VERSION}")
    target_include_directories(help_source_tests PRIVATE src tests)
    target_link_libraries(help_source_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME help_source COMMAND help_source_tests -silent)
    set_tests_properties(help_source PROPERTIES TIMEOUT 60 ENVIRONMENT
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
        if(APPLE)
            # Official Qt kits load libdbus dynamically; Homebrew's Qt links it.
            get_filename_component(test_dbus_bin_dir "${DBUS_DAEMON_EXECUTABLE}" DIRECTORY)
            find_library(SEVENZIP_TEST_DBUS_LIBRARY NAMES dbus-1
                HINTS "${test_dbus_bin_dir}/../lib" REQUIRED)
            get_filename_component(test_dbus_library_dir "${SEVENZIP_TEST_DBUS_LIBRARY}" DIRECTORY)
            set_property(TEST launcher_progress taskbar_adapter APPEND PROPERTY ENVIRONMENT
                "DYLD_LIBRARY_PATH=${test_dbus_library_dir}:$ENV{DYLD_LIBRARY_PATH}")
        endif()
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
    qt_add_executable(source_prompt_presentation_tests tests/source_prompt_presentation_tests.cpp tests/layout_inspection.cpp)
    target_link_libraries(source_prompt_presentation_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME source_prompt_presentation COMMAND source_prompt_presentation_tests -silent)
    set_tests_properties(source_prompt_presentation PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/source-prompt-presentation.ini")
    qt_add_executable(list_view_dialog_tests tests/list_view_dialog_tests.cpp tests/layout_inspection.cpp)
    target_link_libraries(list_view_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME list_view_dialog COMMAND list_view_dialog_tests -silent)
    set_tests_properties(list_view_dialog PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
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
    add_test(NAME linux_frames COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/linux_frames_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime "$<TARGET_FILE:sevenzip_linux_frames>")
    set_tests_properties(linux_frames PROPERTIES TIMEOUT 45)
    qt_add_executable(linux_frames_stream_tests tests/linux_frames_stream_tests.cpp src/linuxframestream.cpp)
    target_include_directories(linux_frames_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(linux_frames_stream_tests PRIVATE sevenzip_panel sevenzip_linux_frames_core Qt6::Test)
    if(WIN32)
        target_link_libraries(linux_frames_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(linux_frames_stream_tests PRIVATE
        "SEVENZIP_FRAME_MODULE=\"$<TARGET_FILE:sevenzip_linux_frames>\""
        "SEVENZIP_FRAME_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_FRAME_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_FRAME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/linux_frames_fixture.py\"")
    add_dependencies(linux_frames_stream_tests sevenzip_linux_frames_runtime)
    add_test(NAME linux_frames_stream COMMAND linux_frames_stream_tests -silent)
    set_tests_properties(linux_frames_stream PROPERTIES TIMEOUT 30)
    add_test(NAME tar COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/tar_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime)
    set_tests_properties(tar PROPERTIES TIMEOUT 45)
    qt_add_executable(tar_stream_tests tests/tar_stream_tests.cpp)
    target_include_directories(tar_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(tar_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(tar_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(tar_stream_tests PRIVATE
        "SEVENZIP_TAR_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_TAR_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_TAR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/tar_fixture.py\"")
    add_test(NAME tar_stream COMMAND tar_stream_tests -silent)
    set_tests_properties(tar_stream PROPERTIES TIMEOUT 30)
    add_test(NAME ihex COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/ihex_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime)
    set_tests_properties(ihex PROPERTIES TIMEOUT 45)
    qt_add_executable(ihex_stream_tests tests/ihex_stream_tests.cpp)
    target_include_directories(ihex_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(ihex_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(ihex_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(ihex_stream_tests PRIVATE
        "SEVENZIP_IHEX_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_IHEX_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_IHEX_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/ihex_fixture.py\"")
    add_test(NAME ihex_stream COMMAND ihex_stream_tests -silent)
    set_tests_properties(ihex_stream PROPERTIES TIMEOUT 30)
    add_test(NAME uefi COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/uefi_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime)
    set_tests_properties(uefi PROPERTIES TIMEOUT 45)
    qt_add_executable(uefi_stream_tests tests/uefi_stream_tests.cpp)
    target_include_directories(uefi_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(uefi_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(uefi_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(uefi_stream_tests PRIVATE
        "SEVENZIP_UEFI_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_UEFI_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_UEFI_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/uefi_fixture.py\"")
    add_test(NAME uefi_stream COMMAND uefi_stream_tests -silent)
    set_tests_properties(uefi_stream PROPERTIES TIMEOUT 30)
    add_test(NAME pe COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/pe_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime)
    set_tests_properties(pe PROPERTIES TIMEOUT 45)
    qt_add_executable(pe_stream_tests tests/pe_stream_tests.cpp)
    target_include_directories(pe_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(pe_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(pe_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(pe_stream_tests PRIVATE
        "SEVENZIP_PE_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_PE_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_PE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/pe_fixture.py\"")
    add_test(NAME pe_stream COMMAND pe_stream_tests -silent)
    set_tests_properties(pe_stream PROPERTIES TIMEOUT 30)
    add_test(NAME elf COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/elf_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime)
    set_tests_properties(elf PROPERTIES TIMEOUT 45)
    qt_add_executable(elf_stream_tests tests/elf_stream_tests.cpp)
    target_include_directories(elf_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(elf_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(elf_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(elf_stream_tests PRIVATE
        "SEVENZIP_ELF_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_ELF_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_ELF_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/elf_fixture.py\"")
    add_test(NAME elf_stream COMMAND elf_stream_tests -silent)
    set_tests_properties(elf_stream PROPERTIES TIMEOUT 30)
    add_test(NAME base64 COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/base64_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime)
    set_tests_properties(base64 PROPERTIES TIMEOUT 45)
    qt_add_executable(base64_stream_tests tests/base64_stream_tests.cpp)
    target_include_directories(base64_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(base64_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(base64_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(base64_stream_tests PRIVATE
        "SEVENZIP_BASE64_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_BASE64_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_BASE64_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/base64_fixture.py\"")
    add_test(NAME base64_stream COMMAND base64_stream_tests -silent)
    set_tests_properties(base64_stream PROPERTIES TIMEOUT 30)
    add_test(NAME zstd COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/zstd_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime)
    set_tests_properties(zstd PROPERTIES TIMEOUT 45)
    qt_add_executable(zstd_stream_tests tests/zstd_stream_tests.cpp)
    target_include_directories(zstd_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(zstd_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(zstd_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(zstd_stream_tests PRIVATE
        "SEVENZIP_ZSTD_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_ZSTD_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_ZSTD_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/zstd_fixture.py\"")
    add_test(NAME zstd_stream COMMAND zstd_stream_tests -silent)
    set_tests_properties(zstd_stream PROPERTIES TIMEOUT 30)
    add_test(NAME xz COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/xz_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime)
    set_tests_properties(xz PROPERTIES TIMEOUT 45)
    qt_add_executable(xz_stream_tests tests/xz_stream_tests.cpp)
    target_include_directories(xz_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(xz_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(xz_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(xz_stream_tests PRIVATE
        "SEVENZIP_XZ_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_XZ_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_XZ_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/xz_fixture.py\"")
    add_test(NAME xz_stream COMMAND xz_stream_tests -silent)
    set_tests_properties(xz_stream PROPERTIES TIMEOUT 30)
    add_test(NAME bzip2 COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/bzip2_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime)
    set_tests_properties(bzip2 PROPERTIES TIMEOUT 45)
    qt_add_executable(bzip2_stream_tests tests/bzip2_stream_tests.cpp)
    target_include_directories(bzip2_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(bzip2_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(bzip2_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(bzip2_stream_tests PRIVATE
        "SEVENZIP_BZIP2_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_BZIP2_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_BZIP2_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/bzip2_fixture.py\"")
    add_test(NAME bzip2_stream COMMAND bzip2_stream_tests -silent)
    set_tests_properties(bzip2_stream PROPERTIES TIMEOUT 30)
    add_test(NAME lzma COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/lzma_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime)
    set_tests_properties(lzma PROPERTIES TIMEOUT 45)
    qt_add_executable(lzma_stream_tests tests/lzma_stream_tests.cpp)
    target_include_directories(lzma_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(lzma_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(lzma_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(lzma_stream_tests PRIVATE
        "SEVENZIP_LZMA_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_LZMA_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_LZMA_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/lzma_fixture.py\"")
    add_test(NAME lzma_stream COMMAND lzma_stream_tests -silent)
    set_tests_properties(lzma_stream PROPERTIES TIMEOUT 30)
    add_test(NAME gzip COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/gzip_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime)
    set_tests_properties(gzip PROPERTIES TIMEOUT 45)
    qt_add_executable(gzip_stream_tests tests/gzip_stream_tests.cpp)
    target_include_directories(gzip_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(gzip_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(gzip_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(gzip_stream_tests PRIVATE
        "SEVENZIP_GZIP_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_GZIP_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_GZIP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/gzip_fixture.py\"")
    add_test(NAME gzip_stream COMMAND gzip_stream_tests -silent)
    set_tests_properties(gzip_stream PROPERTIES TIMEOUT 30)
    add_test(NAME cpio COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/cpio_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime "$<TARGET_FILE:sevenzip_linux_frames>")
    set_tests_properties(cpio PROPERTIES TIMEOUT 45)
    add_test(NAME initramfs COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/initramfs_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime "$<TARGET_FILE:sevenzip_linux_frames>")
    set_tests_properties(initramfs PROPERTIES TIMEOUT 60)
    qt_add_executable(cpio_stream_tests tests/cpio_stream_tests.cpp)
    target_include_directories(cpio_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(cpio_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(cpio_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(cpio_stream_tests PRIVATE
        "SEVENZIP_CPIO_MODULE=\"$<TARGET_FILE:sevenzip_linux_frames>\""
        "SEVENZIP_INITRAMFS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/initramfs_fixture.py\""
        "SEVENZIP_CPIO_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_CPIO_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_CPIO_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/cpio_fixture.py\"")
    add_test(NAME cpio_stream COMMAND cpio_stream_tests -silent)
    add_dependencies(cpio_stream_tests sevenzip_linux_frames_runtime)
    set_tests_properties(cpio_stream PROPERTIES TIMEOUT 30)
    add_test(NAME mbr COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/mbr_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime "$<TARGET_FILE:sevenzip_mbr>"
        "$<TARGET_FILE:sevenzip_fat>")
    set_tests_properties(mbr PROPERTIES TIMEOUT 45)
    qt_add_executable(mbr_stream_tests tests/mbr_stream_tests.cpp)
    target_include_directories(mbr_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(mbr_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(mbr_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(mbr_stream_tests PRIVATE
        "SEVENZIP_MBR_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_MBR_MODULE=\"$<TARGET_FILE:sevenzip_mbr>\""
        "SEVENZIP_MBR_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_MBR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/mbr_fixture.py\"")
    target_compile_definitions(mbr_stream_tests PRIVATE
        "SEVENZIP_MBR_SUBTYPE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/mbr_subtypes_fixture.py\"")
    target_compile_definitions(mbr_stream_tests PRIVATE
        "SEVENZIP_MBR_CLASSIFICATION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/mbr_classification_fixture.py\"")
    add_test(NAME mbr_stream COMMAND mbr_stream_tests -silent)
    set_tests_properties(mbr_stream PROPERTIES TIMEOUT 45)
    add_dependencies(mbr_stream_tests sevenzip_mbr_runtime sevenzip_fat_runtime)
    add_test(NAME apm COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/apm_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime)
    set_tests_properties(apm PROPERTIES TIMEOUT 45)
    qt_add_executable(apm_stream_tests tests/apm_stream_tests.cpp)
    target_include_directories(apm_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(apm_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(apm_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(apm_stream_tests PRIVATE
        "SEVENZIP_APM_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_APM_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_APM_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/apm_fixture.py\"")
    add_test(NAME apm_stream COMMAND apm_stream_tests -silent)
    set_tests_properties(apm_stream PROPERTIES TIMEOUT 30)
    add_test(NAME gpt COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/gpt_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime)
    set_tests_properties(gpt PROPERTIES TIMEOUT 45)
    qt_add_executable(gpt_stream_tests tests/gpt_stream_tests.cpp)
    target_include_directories(gpt_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(gpt_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(gpt_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(gpt_stream_tests PRIVATE
        "SEVENZIP_GPT_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_GPT_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_GPT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/gpt_fixture.py\"")
    add_test(NAME gpt_stream COMMAND gpt_stream_tests -silent)
    set_tests_properties(gpt_stream PROPERTIES TIMEOUT 30)
    add_test(NAME squashfs COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/squashfs_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime "$<TARGET_FILE:sevenzip_squashfs>")
    set_tests_properties(squashfs PROPERTIES TIMEOUT 60)
    qt_add_executable(squashfs_stream_tests tests/squashfs_stream_tests.cpp)
    target_include_directories(squashfs_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(squashfs_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(squashfs_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(squashfs_stream_tests PRIVATE
        "SEVENZIP_SQUASHFS_CORE=\"$<TARGET_FILE:sevenzip_squashfs>\""
        "SEVENZIP_SQUASHFS_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_SQUASHFS_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_SQUASHFS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/squashfs_fixture.py\"")
    add_test(NAME squashfs_stream COMMAND squashfs_stream_tests -silent)
    add_dependencies(squashfs_stream_tests sevenzip_squashfs_runtime)
    set_tests_properties(squashfs_stream PROPERTIES TIMEOUT 45)
    qt_add_executable(ntfs_stream_tests tests/ntfs_stream_tests.cpp)
    target_include_directories(ntfs_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(ntfs_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(ntfs_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(ntfs_stream_tests PRIVATE
        "SEVENZIP_NTFS_CORE=\"$<TARGET_FILE:sevenzip_ntfs>\""
        "SEVENZIP_NTFS_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_NTFS_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_NTFS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/ntfs_fixture.py\"")
    add_test(NAME ntfs_stream COMMAND ntfs_stream_tests -silent)
    add_dependencies(ntfs_stream_tests sevenzip_ntfs_runtime)
    set_tests_properties(ntfs_stream PROPERTIES TIMEOUT 60)
    qt_add_executable(ntfs_directory_tests tests/ntfs_directory_tests.cpp tests/ntfs_test_support.h)
    target_include_directories(ntfs_directory_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(ntfs_directory_tests PRIVATE sevenzip_filesystem Qt6::Test)
    if(WIN32)
        target_link_libraries(ntfs_directory_tests PRIVATE oleaut32)
    endif()
    get_target_property(ntfs_test_definitions ntfs_stream_tests COMPILE_DEFINITIONS)
    target_compile_definitions(ntfs_directory_tests PRIVATE ${ntfs_test_definitions})
    add_test(NAME ntfs_directory COMMAND ntfs_directory_tests -silent)
    add_dependencies(ntfs_directory_tests sevenzip_ntfs_runtime)
    set_tests_properties(ntfs_directory PROPERTIES TIMEOUT 45)
    qt_add_executable(ntfs_record_tests tests/ntfs_record_tests.cpp tests/ntfs_test_support.h)
    target_include_directories(ntfs_record_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(ntfs_record_tests PRIVATE sevenzip_filesystem Qt6::Test)
    if(WIN32)
        target_link_libraries(ntfs_record_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(ntfs_record_tests PRIVATE ${ntfs_test_definitions})
    add_test(NAME ntfs_record COMMAND ntfs_record_tests -silent)
    add_dependencies(ntfs_record_tests sevenzip_ntfs_runtime)
    set_tests_properties(ntfs_record PROPERTIES TIMEOUT 45)
    qt_add_executable(ntfs_deleted_tests tests/ntfs_deleted_tests.cpp tests/ntfs_test_support.h)
    target_include_directories(ntfs_deleted_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(ntfs_deleted_tests PRIVATE sevenzip_filesystem Qt6::Test)
    if(WIN32)
        target_link_libraries(ntfs_deleted_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(ntfs_deleted_tests PRIVATE ${ntfs_test_definitions})
    add_test(NAME ntfs_deleted COMMAND ntfs_deleted_tests -silent)
    add_dependencies(ntfs_deleted_tests sevenzip_ntfs_runtime)
    set_tests_properties(ntfs_deleted PROPERTIES TIMEOUT 45)
    qt_add_executable(ntfs_security_list_tests tests/ntfs_security_list_tests.cpp tests/ntfs_test_support.h)
    target_include_directories(ntfs_security_list_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(ntfs_security_list_tests PRIVATE sevenzip_filesystem Qt6::Test)
    if(WIN32)
        target_link_libraries(ntfs_security_list_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(ntfs_security_list_tests PRIVATE ${ntfs_test_definitions})
    add_test(NAME ntfs_security_list COMMAND ntfs_security_list_tests -silent)
    add_dependencies(ntfs_security_list_tests sevenzip_ntfs_runtime)
    set_tests_properties(ntfs_security_list PROPERTIES TIMEOUT 45)
    qt_add_executable(ntfs_security_tests tests/ntfs_security_tests.cpp)
    target_include_directories(ntfs_security_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(ntfs_security_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(ntfs_security_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(ntfs_security_tests PRIVATE
        "SEVENZIP_NTFS_MODULE=\"$<TARGET_FILE:sevenzip_ntfs>\""
        "SEVENZIP_NTFS_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_NTFS_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_NTFS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/ntfs_fixture.py\"")
    add_dependencies(ntfs_security_tests sevenzip_ntfs_runtime)
    add_test(NAME ntfs_security COMMAND ntfs_security_tests -silent)
    set_tests_properties(ntfs_security PROPERTIES TIMEOUT 60)
    add_test(NAME hfs COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/hfs_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime "$<TARGET_FILE:sevenzip_hfs>")
    set_tests_properties(hfs PROPERTIES TIMEOUT 60)
    qt_add_executable(hfs_stream_tests tests/hfs_stream_tests.cpp)
    target_include_directories(hfs_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(hfs_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(hfs_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(hfs_stream_tests PRIVATE
        "SEVENZIP_HFS_CORE=\"$<TARGET_FILE:sevenzip_hfs>\""
        "SEVENZIP_HFS_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_HFS_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_HFS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/hfs_fixture.py\"")
    add_test(NAME hfs_stream COMMAND hfs_stream_tests -silent)
    add_dependencies(hfs_stream_tests sevenzip_hfs_runtime)
    set_tests_properties(hfs_stream PROPERTIES TIMEOUT 45)
    add_test(NAME udf COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/udf_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime "$<TARGET_FILE:sevenzip_udf>")
    set_tests_properties(udf PROPERTIES TIMEOUT 60)
    qt_add_executable(udf_stream_tests tests/udf_stream_tests.cpp)
    target_include_directories(udf_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(udf_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(udf_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(udf_stream_tests PRIVATE
        "SEVENZIP_UDF_CORE=\"$<TARGET_FILE:sevenzip_udf>\""
        "SEVENZIP_UDF_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_UDF_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_UDF_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/udf_fixture.py\"")
    add_test(NAME udf_stream COMMAND udf_stream_tests -silent)
    add_dependencies(udf_stream_tests sevenzip_udf_runtime)
    set_tests_properties(udf_stream PROPERTIES TIMEOUT 45)
    add_test(NAME iso COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/iso_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime "$<TARGET_FILE:sevenzip_iso>")
    set_tests_properties(iso PROPERTIES TIMEOUT 60)
    qt_add_executable(iso_stream_tests tests/iso_stream_tests.cpp)
    target_include_directories(iso_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(iso_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(iso_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(iso_stream_tests PRIVATE
        "SEVENZIP_ISO_CORE=\"$<TARGET_FILE:sevenzip_iso>\""
        "SEVENZIP_ISO_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_ISO_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_ISO_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/iso_fixture.py\"")
    add_test(NAME iso_stream COMMAND iso_stream_tests -silent)
    add_dependencies(iso_stream_tests sevenzip_iso_runtime)
    set_tests_properties(iso_stream PROPERTIES TIMEOUT 60)
    add_test(NAME ntfs COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/ntfs_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime "$<TARGET_FILE:sevenzip_ntfs>")
    set_tests_properties(ntfs PROPERTIES TIMEOUT 60)
    qt_add_executable(squashfs_metadata_tests tests/squashfs_metadata_tests.cpp)
    target_include_directories(squashfs_metadata_tests PRIVATE src)
    target_link_libraries(squashfs_metadata_tests PRIVATE sevenzip_squashfs_metadata Qt6::Test)
    target_compile_definitions(squashfs_metadata_tests PRIVATE
        "SEVENZIP_SQUASHFS_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_SQUASHFS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/squashfs_fixture.py\"")
    add_test(NAME squashfs_metadata COMMAND squashfs_metadata_tests -silent)
    set_tests_properties(squashfs_metadata PROPERTIES TIMEOUT 30)
    add_test(NAME fat COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fat_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime "$<TARGET_FILE:sevenzip_fat>")
    set_tests_properties(fat PROPERTIES TIMEOUT 60)
    add_test(NAME ext COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/ext_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime "$<TARGET_FILE:sevenzip_ext>")
    set_tests_properties(ext PROPERTIES TIMEOUT 45)
    qt_add_executable(ext_stream_tests tests/ext_stream_tests.cpp)
    target_include_directories(ext_stream_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(ext_stream_tests PRIVATE sevenzip_filesystem Qt6::Test)
    if(WIN32)
        target_link_libraries(ext_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(ext_stream_tests PRIVATE
        "SEVENZIP_EXT_CORE=\"$<TARGET_FILE:sevenzip_ext>\""
        "SEVENZIP_EXT_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_EXT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/ext_fixture.py\"")
    add_test(NAME ext_stream COMMAND ext_stream_tests -silent)
    add_dependencies(ext_stream_tests sevenzip_ext_runtime)
    set_tests_properties(ext_stream PROPERTIES TIMEOUT 45)
    qt_add_executable(ext_fscrypt_tests tests/ext_fscrypt_tests.cpp)
    target_include_directories(ext_fscrypt_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(ext_fscrypt_tests PRIVATE sevenzip_filesystem Qt6::Test)
    if(WIN32)
        target_link_libraries(ext_fscrypt_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(ext_fscrypt_tests PRIVATE
        "SEVENZIP_EXT_CORE=\"$<TARGET_FILE:sevenzip_ext>\""
        "SEVENZIP_FSCRYPT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/fscrypt\"")
    add_dependencies(ext_fscrypt_tests sevenzip_ext_runtime)
    add_test(NAME ext_fscrypt COMMAND ext_fscrypt_tests -silent)
    set_tests_properties(ext_fscrypt PROPERTIES TIMEOUT 30)
    add_test(NAME ext_fscrypt_cli COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/ext_fscrypt_cli_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime "$<TARGET_FILE:sevenzip_ext>")
    set_tests_properties(ext_fscrypt_cli PROPERTIES TIMEOUT 30)
    qt_add_executable(ext_fscrypt_backend_tests tests/ext_fscrypt_backend_tests.cpp)
    target_include_directories(ext_fscrypt_backend_tests PRIVATE src)
    target_link_libraries(ext_fscrypt_backend_tests PRIVATE sevenzip_panel Qt6::Test)
    target_compile_definitions(ext_fscrypt_backend_tests PRIVATE
        "SEVENZIP_EXT_CORE=\"$<TARGET_FILE:sevenzip_ext>\""
        "SEVENZIP_EXT_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_FSCRYPT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/fscrypt\"")
    add_dependencies(ext_fscrypt_backend_tests sevenzip_ext_runtime)
    add_test(NAME ext_fscrypt_backend COMMAND ext_fscrypt_backend_tests -silent)
    set_tests_properties(ext_fscrypt_backend PROPERTIES TIMEOUT 45)
    qt_add_executable(ext_xattr_backend_tests tests/ext_xattr_backend_tests.cpp)
    target_include_directories(ext_xattr_backend_tests PRIVATE src)
    target_link_libraries(ext_xattr_backend_tests PRIVATE sevenzip_panel Qt6::Test)
    target_compile_definitions(ext_xattr_backend_tests PRIVATE
        "SEVENZIP_EXT_CORE=\"$<TARGET_FILE:sevenzip_ext>\""
        "SEVENZIP_EXT_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_EXT_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_EXT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/ext_fixture.py\"")
    add_dependencies(ext_xattr_backend_tests sevenzip_ext_runtime)
    add_test(NAME ext_xattr_backend COMMAND ext_xattr_backend_tests -silent)
    set_tests_properties(ext_xattr_backend PROPERTIES TIMEOUT 45)
    add_test(NAME cramfs COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/cramfs_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime "$<TARGET_FILE:sevenzip_cramfs>")
    set_tests_properties(cramfs PROPERTIES TIMEOUT 45)
    add_test(NAME erofs COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/erofs_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime "$<TARGET_FILE:sevenzip_erofs>")
    set_tests_properties(erofs PROPERTIES TIMEOUT 90)
    add_test(NAME runtime_package COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/package_tests.py)
    add_test(NAME windows_package_verification COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/windows_package_tests.py)
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
    qt_add_executable(panel_folder_tests tests/panel_folder_tests.cpp)
    target_link_libraries(panel_folder_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME panel_folder COMMAND panel_folder_tests -silent)
    set_tests_properties(panel_folder PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_test(NAME ui_parity_tool COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_parity_tests.py)
    add_test(NAME ui_runtime_comparison COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_runtime_comparison_tests.py)
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
    qt_add_executable(disclosure_accessibility_tests tests/disclosure_accessibility_tests.cpp)
    target_link_libraries(disclosure_accessibility_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME disclosure_accessibility COMMAND disclosure_accessibility_tests -silent)
    set_tests_properties(disclosure_accessibility PROPERTIES TIMEOUT 15
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(context_dialog_tests tests/context_dialog_tests.cpp tests/layout_inspection.cpp
        src/CPP/7zip/UI/FileManager/SplitDialog.cpp)
    target_include_directories(context_dialog_tests PRIVATE tests)
    target_link_libraries(context_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    if(APPLE)
        target_sources(context_dialog_tests PRIVATE tests/fixtures/native_sheet_mac.mm)
    endif()
    add_test(NAME context_dialog COMMAND context_dialog_tests -silent)
    set_tests_properties(context_dialog PROPERTIES TIMEOUT 60 RESOURCE_LOCK sevenzip_native_ui
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    if(APPLE)
        set_property(TEST context_dialog PROPERTY ENVIRONMENT
            "QT_QPA_PLATFORM=cocoa;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    endif()
    qt_add_executable(browse_dialog_tests tests/browse_dialog_tests.cpp)
    target_link_libraries(browse_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME browse_dialog COMMAND browse_dialog_tests -silent)
    set_tests_properties(browse_dialog PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)
    qt_add_executable(source_browse_dialog_tests tests/source_browse_dialog_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(source_browse_dialog_tests PRIVATE tests)
    target_link_libraries(source_browse_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME source_browse_dialog COMMAND source_browse_dialog_tests -silent)
    set_tests_properties(source_browse_dialog PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(combo_dialog PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(volume_dialog_tests tests/volume_dialog_tests.cpp tests/layout_inspection.cpp
        tests/fixtures/split_dialog_prompt.cpp
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
    if(WIN32)
        add_library(native_module_path_fixture SHARED tests/fixtures/native_module_path_fixture.cpp)
        target_include_directories(native_module_path_fixture PRIVATE src)
        target_link_libraries(native_module_path_fixture PRIVATE sevenzip_common)
        target_compile_definitions(native_library_tests PRIVATE
            "NATIVE_MODULE_PATH_FIXTURE=\"$<TARGET_FILE:native_module_path_fixture>\"")
        add_dependencies(native_library_tests native_module_path_fixture)
    endif()
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
    if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
        qt_add_executable(process_snapshot_tests tests/process_snapshot_tests.cpp
            src/qtexternalprocess.cpp)
        target_include_directories(process_snapshot_tests PRIVATE src)
        target_link_libraries(process_snapshot_tests PRIVATE sevenzip_common Qt6::Test)
        add_test(NAME process_snapshot COMMAND process_snapshot_tests -silent)
        set_tests_properties(process_snapshot PROPERTIES TIMEOUT 10)
    endif()
    qt_add_executable(progress_sync_tests tests/progress_sync_tests.cpp)
    target_link_libraries(progress_sync_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME progress_sync COMMAND progress_sync_tests -silent)
    qt_add_executable(worker_checkpoint_tests tests/worker_checkpoint_tests.cpp)
    target_link_libraries(worker_checkpoint_tests PRIVATE sevenzip_progress_sync Qt6::Test)
    add_test(NAME worker_checkpoint COMMAND worker_checkpoint_tests -silent)
    set_tests_properties(worker_checkpoint PROPERTIES TIMEOUT 10)
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
    get_target_property(source_extract_advanced_sources source_extract_backend_tests SOURCES)
    list(REMOVE_ITEM source_extract_advanced_sources tests/source_extract_backend_tests.cpp)
    if(WIN32)
        qt_add_executable(native_mapi_host tests/fixtures/native_mapi_host.cpp
            tests/fixtures/native_mapi_loader.cpp
            src/qtguimain.cpp ${source_extract_advanced_sources}
            src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp
            src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp
            src/createarchivedialog_preferences.cpp)
        target_include_directories(native_mapi_host PRIVATE src)
        target_link_libraries(native_mapi_host PRIVATE sevenzip_location Qt6::Widgets)
        foreach(mapi_variant IN ITEMS wide ansi missing)
            add_library(native_mapi_${mapi_variant} SHARED tests/fixtures/native_mapi_provider.cpp)
            target_link_libraries(native_mapi_${mapi_variant} PRIVATE Qt6::Core)
            set_target_properties(native_mapi_${mapi_variant} PROPERTIES
                OUTPUT_NAME Mapi32 AUTOMOC OFF AUTOUIC OFF
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mapi-fixtures/${mapi_variant}"
                LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mapi-fixtures/${mapi_variant}"
                ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mapi-fixtures/${mapi_variant}")
        endforeach()
        target_compile_definitions(native_mapi_wide PRIVATE SEVENZIP_MAPI_WIDE SEVENZIP_MAPI_ANSI)
        target_compile_definitions(native_mapi_ansi PRIVATE SEVENZIP_MAPI_ANSI)
        target_sources(native_mapi_wide PRIVATE tests/fixtures/native_mapi_wide.def)
        target_sources(native_mapi_ansi PRIVATE tests/fixtures/native_mapi_ansi.def)
        qt_add_executable(native_mapi_tests tests/native_mapi_tests.cpp)
        target_include_directories(native_mapi_tests PRIVATE tests)
        target_link_libraries(native_mapi_tests PRIVATE Qt6::Test)
        target_compile_definitions(native_mapi_tests PRIVATE
            "NATIVE_MAPI_HOST=\"$<TARGET_FILE:native_mapi_host>\""
            "NATIVE_MAPI_WIDE=\"$<TARGET_FILE:native_mapi_wide>\""
            "NATIVE_MAPI_ANSI=\"$<TARGET_FILE:native_mapi_ansi>\""
            "NATIVE_MAPI_MISSING=\"$<TARGET_FILE:native_mapi_missing>\"")
        add_dependencies(native_mapi_tests native_mapi_host native_mapi_wide native_mapi_ansi native_mapi_missing)
        add_test(NAME native_mapi COMMAND native_mapi_tests -silent)
        set_tests_properties(native_mapi PROPERTIES TIMEOUT 180 ENVIRONMENT
            "SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native_mapi.ini")
    endif()
    qt_add_executable(archive_open_options_tests tests/archive_open_options_tests.cpp
        ${source_extract_advanced_sources})
    target_include_directories(archive_open_options_tests PRIVATE src tests)
    target_link_libraries(archive_open_options_tests PRIVATE sevenzip_location Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(archive_open_options_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME archive_open_options COMMAND archive_open_options_tests -silent)
    set_tests_properties(archive_open_options PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/archive-open-options.ini")
    qt_add_executable(source_extract_advanced_tests tests/source_extract_advanced_tests.cpp
        tests/layout_inspection.cpp ${source_extract_advanced_sources})
    target_include_directories(source_extract_advanced_tests PRIVATE src)
    target_link_libraries(source_extract_advanced_tests PRIVATE sevenzip_location Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(source_extract_advanced_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME source_extract_advanced COMMAND source_extract_advanced_tests -silent)
    get_target_property(shell_archive_sources source_extract_advanced_tests SOURCES)
    list(REMOVE_ITEM shell_archive_sources tests/source_extract_advanced_tests.cpp)
    qt_add_executable(shell_archive_services_tests tests/shell_archive_services_tests.cpp
        ${shell_archive_sources})
    target_include_directories(shell_archive_services_tests PRIVATE src tests)
    qt_add_executable(source_shell_update_tests tests/source_shell_update_tests.cpp
        ${shell_archive_sources})
    target_include_directories(source_shell_update_tests PRIVATE src tests)
    target_link_libraries(source_shell_update_tests PRIVATE sevenzip_location Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(source_shell_update_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME source_shell_update COMMAND source_shell_update_tests -silent)
    set_tests_properties(source_shell_update PROPERTIES TIMEOUT 60
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(shell_update_gui_tests tests/shell_update_gui_tests.cpp
        ${shell_archive_sources})
    target_include_directories(shell_update_gui_tests PRIVATE src tests)
    target_link_libraries(shell_update_gui_tests PRIVATE sevenzip_location Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(shell_update_gui_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME shell_update_gui COMMAND shell_update_gui_tests -silent)
    set_tests_properties(shell_update_gui PROPERTIES TIMEOUT 60
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(gui_error_policy_tests tests/gui_error_policy_tests.cpp
        tests/layout_inspection.cpp ${shell_archive_sources})
    target_include_directories(gui_error_policy_tests PRIVATE src tests)
    target_link_libraries(gui_error_policy_tests PRIVATE sevenzip_location Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(gui_error_policy_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME gui_error_policy COMMAND gui_error_policy_tests -silent)
    set_tests_properties(gui_error_policy PROPERTIES TIMEOUT 30
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    target_link_libraries(shell_archive_services_tests PRIVATE sevenzip_location Qt6::Test)
    get_target_property(shell_archive_definitions source_extract_advanced_tests COMPILE_DEFINITIONS)
    if(shell_archive_definitions)
        target_compile_definitions(shell_archive_services_tests PRIVATE ${shell_archive_definitions})
    endif()
    add_test(NAME shell_archive_services COMMAND shell_archive_services_tests -silent)
    set_tests_properties(shell_archive_services PROPERTIES TIMEOUT 60
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(source_link_extract_tests tests/source_link_extract_tests.cpp
        ${source_extract_advanced_sources})
    target_include_directories(source_link_extract_tests PRIVATE src)
    target_link_libraries(source_link_extract_tests PRIVATE sevenzip_location Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(source_link_extract_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME source_link_extract COMMAND source_link_extract_tests -silent)
    set_tests_properties(source_link_extract PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    set_tests_properties(source_extract_advanced PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports" TIMEOUT 60)
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
    if(WIN32)
        qt_add_executable(native_drag_archive_tests tests/native_drag_archive_tests.cpp ${owned_temp_publication_sources})
        target_link_libraries(native_drag_archive_tests PRIVATE sevenzip_location Qt6::Test)
        add_test(NAME native_drag_archive COMMAND native_drag_archive_tests -silent)
        set_tests_properties(native_drag_archive PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    endif()
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
    qt_add_executable(link_dialog_tests tests/link_dialog_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(link_dialog_tests PRIVATE src tests)
    target_link_libraries(link_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME link_dialog COMMAND link_dialog_tests -silent)
    set_tests_properties(link_dialog PROPERTIES TIMEOUT 30 RESOURCE_LOCK sevenzip_ui_artifacts ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/link_dialog.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(benchmark_lifecycle PROPERTIES TIMEOUT 30 RESOURCE_LOCK sevenzip_ui_artifacts ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/benchmark_lifecycle.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(hash_results PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports" TIMEOUT 30)
    qt_add_executable(overwrite_layout_tests tests/overwrite_layout_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(overwrite_layout_tests PRIVATE tests)
    target_link_libraries(overwrite_layout_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME overwrite_layout COMMAND overwrite_layout_tests -silent)
    set_tests_properties(overwrite_layout PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(extract_prompt_tests tests/extract_prompt_tests.cpp tests/layout_inspection.cpp)
    qt_add_executable(language_dialog_tests tests/language_dialog_tests.cpp tests/layout_inspection.cpp
        tests/fixtures/split_dialog_prompt.cpp
        src/CPP/7zip/UI/FileManager/SplitDialog.cpp)
    target_include_directories(language_dialog_tests PRIVATE src tests)
    target_link_libraries(language_dialog_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME language_dialog COMMAND language_dialog_tests -silent)
    qt_add_executable(language_standard_buttons_tests tests/language_standard_buttons_tests.cpp
        tests/layout_inspection.cpp src/CPP/7zip/UI/FileManager/SplitDialog.cpp
        src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp
        src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp
        src/createarchivedialog_preferences.cpp)
    target_include_directories(language_standard_buttons_tests PRIVATE src tests)
    target_link_libraries(language_standard_buttons_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME language_standard_buttons COMMAND language_standard_buttons_tests -silent)
    set_tests_properties(language_standard_buttons PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/language-standard-buttons.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
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
    qt_add_executable(zip_registry_tests tests/zip_registry_tests.cpp)
    target_include_directories(zip_registry_tests PRIVATE src)
    target_link_libraries(zip_registry_tests PRIVATE sevenzip_common Qt6::Test)
    add_test(NAME zip_registry COMMAND zip_registry_tests -silent)
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
    qt_add_executable(agent_opening_tests tests/agent_opening_tests.cpp
        src/CPP/7zip/UI/GUI/CompressDialog.cpp src/archivebackend.cpp
        src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp)
    target_link_libraries(agent_opening_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME agent_opening COMMAND agent_opening_tests -silent)
    set_tests_properties(agent_opening PROPERTIES TIMEOUT 90)
    qt_add_executable(device_folder_tests tests/device_folder_tests.cpp
        src/CPP/7zip/UI/GUI/CompressDialog.cpp src/archivebackend.cpp
        src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp)
    target_link_libraries(device_folder_tests PRIVATE sevenzip_location Qt6::Test)
    target_compile_definitions(device_folder_tests PRIVATE
        "DEVICE_FIXTURE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/devices\"")
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(device_folder_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME device_folder COMMAND device_folder_tests -silent)
    set_tests_properties(device_folder PROPERTIES TIMEOUT 30)
    qt_add_executable(agent_lifecycle_tests tests/agent_lifecycle_tests.cpp
        src/CPP/7zip/UI/GUI/CompressDialog.cpp src/archivebackend.cpp
        src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp)
    target_link_libraries(agent_lifecycle_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME agent_lifecycle COMMAND agent_lifecycle_tests -silent)
    set_tests_properties(agent_lifecycle PROPERTIES TIMEOUT 90)
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
    if(WIN32)
        add_executable(process_pause_allocation_tests tests/process_pause_allocation_tests.cpp)
        target_include_directories(process_pause_allocation_tests PRIVATE src)
        target_link_libraries(process_pause_allocation_tests PRIVATE sevenzip_common)
        add_test(NAME process_pause_allocation COMMAND process_pause_allocation_tests)
        set_tests_properties(process_pause_allocation PROPERTIES TIMEOUT 30)
        add_executable(process_handle_allocation_tests tests/process_handle_allocation_tests.cpp
            src/qtexternalprocess.cpp)
        target_include_directories(process_handle_allocation_tests PRIVATE src)
        target_link_libraries(process_handle_allocation_tests PRIVATE sevenzip_common)
        set_target_properties(process_handle_allocation_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF)
        add_test(NAME process_handle_allocation COMMAND process_handle_allocation_tests)
        set_tests_properties(process_handle_allocation PROPERTIES TIMEOUT 15)
    elseif(UNIX)
        add_executable(process_spawn_allocation_tests tests/process_spawn_allocation_tests.cpp
            src/qtexternalprocess.cpp)
        target_include_directories(process_spawn_allocation_tests PRIVATE src)
        target_link_libraries(process_spawn_allocation_tests PRIVATE sevenzip_common)
        set_target_properties(process_spawn_allocation_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF)
        add_test(NAME process_spawn_allocation COMMAND process_spawn_allocation_tests)
        set_tests_properties(process_spawn_allocation PROPERTIES TIMEOUT 15)
    endif()
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
    if(WIN32)
        qt_add_executable(fs_drives_device_tests tests/fs_drives_device_tests.cpp
            tests/fixtures/read_only_virtual_disk.cpp tests/fixtures/read_only_virtual_disk.h)
        target_link_libraries(fs_drives_device_tests PRIVATE sevenzip_filesystem Qt6::Test virtdisk)
        add_test(NAME fs_drives_device COMMAND fs_drives_device_tests -silent)
        set_tests_properties(fs_drives_device PROPERTIES TIMEOUT 90 RUN_SERIAL TRUE)
        qt_add_executable(network_folder_tests tests/native_network_folder_tests.cpp
            tests/fixtures/native_network_share.cpp tests/fixtures/native_network_share.h)
        target_link_libraries(network_folder_tests PRIVATE netapi32 advapi32)
    else()
        qt_add_executable(network_folder_tests tests/network_folder_tests.cpp)
    endif()
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
    qt_add_executable(dialog_size_tests tests/dialog_size_tests.cpp)
    target_include_directories(dialog_size_tests PRIVATE src)
    target_link_libraries(dialog_size_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME dialog_size COMMAND dialog_size_tests -silent)
    set_tests_properties(dialog_size PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(copy_dialog_tests tests/copy_dialog_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(copy_dialog_tests PRIVATE src tests)
    target_link_libraries(copy_dialog_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_panel)
    add_test(NAME copy_dialog COMMAND copy_dialog_tests -silent)
    set_tests_properties(copy_dialog PROPERTIES FIXTURES_SETUP ui_reference_copy)
    set_tests_properties(copy_dialog PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/copy-dialog-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports" TIMEOUT 30)
    qt_add_executable(archive_backend_tests tests/archive_backend_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h)
    qt_add_executable(archive_mutation_tests tests/archive_mutation_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h)
    get_target_property(archive_preparation_sources archive_mutation_tests SOURCES)
    list(REMOVE_ITEM archive_preparation_sources tests/archive_mutation_tests.cpp)
    qt_add_executable(process_notification_tests tests/process_notification_tests.cpp ${archive_preparation_sources})
    target_include_directories(process_notification_tests PRIVATE src)
    target_link_libraries(process_notification_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME process_notification COMMAND process_notification_tests -silent)
    set_tests_properties(process_notification PROPERTIES TIMEOUT 30)
    qt_add_executable(archive_preparation_tests tests/archive_preparation_tests.cpp ${archive_preparation_sources})
    target_include_directories(archive_preparation_tests PRIVATE src)
    target_link_libraries(archive_preparation_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME archive_preparation COMMAND archive_preparation_tests -silent)
    set_tests_properties(archive_preparation PROPERTIES TIMEOUT 30)
    qt_add_executable(worker_progress_tests tests/worker_progress_tests.cpp ${archive_preparation_sources})
    target_include_directories(worker_progress_tests PRIVATE src)
    target_link_libraries(worker_progress_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME worker_progress COMMAND worker_progress_tests -silent)
    set_tests_properties(worker_progress PROPERTIES TIMEOUT 30)
    target_include_directories(archive_backend_tests PRIVATE src)
    target_include_directories(archive_mutation_tests PRIVATE src tests)
    target_link_libraries(archive_backend_tests PRIVATE Qt6::Test)
    target_link_libraries(archive_mutation_tests PRIVATE Qt6::Test)
    qt_add_executable(ui_layout_tests tests/ui_layout_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp  src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_rename.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h  src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(ui_layout_tests PRIVATE src)
    target_link_libraries(ui_layout_tests PRIVATE Qt6::Widgets Qt6::Test)
    qt_add_executable(ui_state_tests tests/ui_state_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp  src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_rename.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h  src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    qt_add_executable(ui_operations_tests tests/ui_operations_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp  src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_rename.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h  src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    qt_add_executable(ui_navigation_tests tests/ui_navigation_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp  src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_rename.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h  src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    qt_add_executable(ui_archive_states_tests tests/ui_archive_states_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp  src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_rename.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h  src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(ui_state_tests PRIVATE src)
    target_include_directories(ui_operations_tests PRIVATE src tests)
    target_include_directories(ui_navigation_tests PRIVATE src tests)
    target_include_directories(ui_archive_states_tests PRIVATE src tests)
    target_sources(ui_archive_states_tests PRIVATE tests/layout_inspection.cpp)
    target_link_libraries(ui_state_tests PRIVATE Qt6::Widgets Qt6::Test)
    target_link_libraries(ui_operations_tests PRIVATE Qt6::Widgets Qt6::Test)
    get_target_property(copy_same_window_sources ui_operations_tests SOURCES)
    list(REMOVE_ITEM copy_same_window_sources tests/ui_operations_tests.cpp)
    qt_add_executable(ui_copy_same_tests tests/ui_copy_same_tests.cpp tests/layout_inspection.cpp
        ${copy_same_window_sources})
    target_include_directories(ui_copy_same_tests PRIVATE src tests)
    target_link_libraries(ui_copy_same_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME ui_copy_same COMMAND ui_copy_same_tests -silent)
    set_tests_properties(ui_copy_same PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/ui-copy-same-settings.ini")
    qt_add_executable(ui_copy_routing_tests tests/ui_copy_routing_tests.cpp tests/layout_inspection.cpp
        ${copy_same_window_sources})
    target_include_directories(ui_copy_routing_tests PRIVATE src tests)
    target_link_libraries(ui_copy_routing_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME ui_copy_routing COMMAND ui_copy_routing_tests -silent)
    set_tests_properties(ui_copy_routing PROPERTIES TIMEOUT 120 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/ui-copy-routing-settings.ini")
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
    add_test(NAME nested_edit_chain COMMAND nested_edit_chain_tests -v1)
    set_tests_properties(nested_edit_chain PROPERTIES TIMEOUT 120 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/nested_edit_chain.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(verctrl PROPERTIES TIMEOUT 90 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/verctrl.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(archive_external_edit_tests tests/archive_external_edit_tests.cpp
        tests/external_editor_fixture.cpp tests/layout_inspection.cpp ${copy_same_window_sources})
    target_include_directories(archive_external_edit_tests PRIVATE src tests)
    target_link_libraries(archive_external_edit_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME archive_external_edit COMMAND archive_external_edit_tests -v1)
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
    qt_add_executable(external_gui_call_tests tests/external_gui_call_tests.cpp)
    target_include_directories(external_gui_call_tests PRIVATE src)
    target_link_libraries(external_gui_call_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME external_gui_call COMMAND external_gui_call_tests -silent)
    set_tests_properties(external_gui_call PROPERTIES TIMEOUT 15 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
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
    qt_add_executable(flat_view_tests tests/flat_view_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp  src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_rename.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h  src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    qt_add_executable(settings_tests tests/settings_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp  src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_rename.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h  src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(settings_tests PRIVATE src)
    target_link_libraries(settings_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME settings COMMAND settings_tests -silent)
    set_tests_properties(settings PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/settings-tests.ini")
    qt_add_executable(properties_tests tests/properties_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp  src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_rename.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h  src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(properties_tests PRIVATE src)
    target_link_libraries(properties_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME properties COMMAND properties_tests -silent)
    qt_add_executable(checksum_tests tests/checksum_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp  src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_rename.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h  src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
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
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SCREENSHOT_DIR=${CMAKE_BINARY_DIR}/ui-screenshots;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/ui-layout-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports"
    )
    add_test(NAME archive_backend COMMAND archive_backend_tests)
    add_test(NAME archive_mutation COMMAND archive_mutation_tests)
    qt_add_executable(extract_dialog_tests tests/extract_dialog_tests.cpp tests/layout_inspection.cpp
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
    target_sources(exfat_backend_tests PRIVATE ${compression_order_sources})
    target_sources(cramfs_module_tests PRIVATE ${compression_order_sources})
    target_sources(squashfs_stream_tests PRIVATE ${compression_order_sources})
    target_sources(ntfs_stream_tests PRIVATE ${compression_order_sources})
    target_sources(ntfs_security_tests PRIVATE ${compression_order_sources})
    target_sources(hfs_stream_tests PRIVATE ${compression_order_sources})
    target_sources(ext_xattr_backend_tests PRIVATE ${compression_order_sources})
    target_sources(ext_fscrypt_backend_tests PRIVATE ${compression_order_sources})
    target_sources(iso_stream_tests PRIVATE ${compression_order_sources})
    target_sources(udf_stream_tests PRIVATE ${compression_order_sources})
    target_sources(gpt_stream_tests PRIVATE ${compression_order_sources})
    target_sources(cpio_stream_tests PRIVATE ${compression_order_sources})
    target_sources(gzip_stream_tests PRIVATE ${compression_order_sources})
    target_sources(lzma_stream_tests PRIVATE ${compression_order_sources})
    target_sources(bzip2_stream_tests PRIVATE ${compression_order_sources})
    target_sources(xz_stream_tests PRIVATE ${compression_order_sources})
    target_sources(zstd_stream_tests PRIVATE ${compression_order_sources})
    target_sources(base64_stream_tests PRIVATE ${compression_order_sources})
    target_sources(elf_stream_tests PRIVATE ${compression_order_sources})
    target_sources(pe_stream_tests PRIVATE ${compression_order_sources})
    target_sources(uefi_stream_tests PRIVATE ${compression_order_sources})
    target_sources(ihex_stream_tests PRIVATE ${compression_order_sources})
    target_sources(tar_stream_tests PRIVATE ${compression_order_sources})
    target_sources(linux_frames_stream_tests PRIVATE ${compression_order_sources})
    target_sources(apm_stream_tests PRIVATE ${compression_order_sources})
    target_sources(fat_module_tests PRIVATE ${compression_order_sources})
    target_sources(erofs_module_tests PRIVATE ${compression_order_sources})
    target_sources(brotli_tests PRIVATE ${compression_order_sources})
    qt_add_executable(compression_error_policy_tests tests/compression_error_policy_tests.cpp
        tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(compression_error_policy_tests PRIVATE src tests)
    target_link_libraries(compression_error_policy_tests PRIVATE sevenzip_panel Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(compression_error_policy_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME compression_error_policy COMMAND compression_error_policy_tests -silent)
    set_tests_properties(compression_error_policy PROPERTIES TIMEOUT 60
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(dialog_path_rules_tests tests/dialog_path_rules_tests.cpp
        tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(dialog_path_rules_tests PRIVATE src tests)
    target_link_libraries(dialog_path_rules_tests PRIVATE sevenzip_panel Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(dialog_path_rules_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME dialog_path_rules COMMAND dialog_path_rules_tests -silent)
    set_tests_properties(dialog_path_rules PROPERTIES TIMEOUT 60
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(compression_advanced_tests tests/compression_advanced_tests.cpp
        tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(compression_advanced_tests PRIVATE src tests)
    target_link_libraries(compression_advanced_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_panel)
    add_test(NAME compression_advanced COMMAND compression_advanced_tests -silent)
    set_tests_properties(compression_advanced PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(compress_options_presentation_tests tests/compress_options_presentation_tests.cpp
        tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(compress_options_presentation_tests PRIVATE src tests)
    target_link_libraries(compress_options_presentation_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME compress_options_presentation COMMAND compress_options_presentation_tests -silent)
    set_tests_properties(compress_options_presentation PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(archive_volume_creation_tests tests/archive_volume_creation_tests.cpp
        tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(archive_volume_creation_tests PRIVATE src tests)
    target_link_libraries(archive_volume_creation_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_panel)
    add_test(NAME archive_volume_creation COMMAND archive_volume_creation_tests -silent)
    add_executable(multiout_lifecycle_tests tests/multiout_lifecycle_tests.cpp)
    target_link_libraries(multiout_lifecycle_tests PRIVATE sevenzip_filesystem)
    add_test(NAME multiout_lifecycle COMMAND multiout_lifecycle_tests)
    set_tests_properties(multiout_lifecycle PROPERTIES TIMEOUT 15)
    qt_add_executable(file_output_lifecycle_tests tests/file_output_lifecycle_tests.cpp)
    target_link_libraries(file_output_lifecycle_tests PRIVATE sevenzip_filesystem Qt6::Test)
    add_test(NAME file_output_lifecycle COMMAND file_output_lifecycle_tests -silent)
    set_tests_properties(file_output_lifecycle PROPERTIES TIMEOUT 15)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_executable(linux_file_close_tests tests/linux_file_close_tests.cpp)
        target_link_libraries(linux_file_close_tests PRIVATE sevenzip_filesystem)
        add_test(NAME linux_file_close COMMAND linux_file_close_tests)
        set_tests_properties(linux_file_close PROPERTIES TIMEOUT 15)
    endif()
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
    qt_add_executable(source_archive_browse_tests tests/source_archive_browse_tests.cpp ${compression_order_sources})
    target_include_directories(source_archive_browse_tests PRIVATE src tests)
    target_link_libraries(source_archive_browse_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(source_archive_browse_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME source_archive_browse COMMAND source_archive_browse_tests -silent)
    set_tests_properties(source_archive_browse PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
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
    qt_add_executable(compression_order_tests tests/compression_order_tests.cpp
        tests/layout_inspection.cpp ${compression_order_sources})
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
    qt_add_executable(compression_profile_selection_tests tests/compression_profile_selection_tests.cpp
        tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(compression_profile_selection_tests PRIVATE src tests)
    target_link_libraries(compression_profile_selection_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME compression_profile_selection COMMAND compression_profile_selection_tests -silent)
    set_tests_properties(compression_profile_selection PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(compression_method_state_tests tests/compression_method_state_tests.cpp
        tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(compression_method_state_tests PRIVATE src tests)
    target_link_libraries(compression_method_state_tests PRIVATE sevenzip_panel Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(compression_method_state_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME compression_method_state COMMAND compression_method_state_tests -silent)
    set_tests_properties(compression_method_state PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(compression_event_tests tests/compression_event_tests.cpp
        tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(compression_event_tests PRIVATE src tests)
    target_link_libraries(compression_event_tests PRIVATE sevenzip_panel Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(compression_event_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME compression_event COMMAND compression_event_tests -silent)
    set_tests_properties(compression_event PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(compression_timestamp_tests tests/compression_timestamp_tests.cpp
        tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(compression_timestamp_tests PRIVATE src tests)
    target_link_libraries(compression_timestamp_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME compression_timestamp COMMAND compression_timestamp_tests -silent)
    set_tests_properties(compression_timestamp PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(compression_format_tests tests/compression_format_tests.cpp
        tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(compression_format_tests PRIVATE src tests)
    target_link_libraries(compression_format_tests PRIVATE sevenzip_location Qt6::Test)
    target_compile_definitions(compression_format_tests PRIVATE Z7_EXTERNAL_CODECS)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(compression_format_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME compression_format COMMAND compression_format_tests -silent)
    set_tests_properties(compression_format PROPERTIES TIMEOUT 60 ENVIRONMENT
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
    qt_add_executable(source_compress_dialog_tests tests/source_compress_dialog_tests.cpp
        tests/layout_inspection.cpp ${compression_order_sources})
    target_include_directories(source_compress_dialog_tests PRIVATE src tests)
    target_link_libraries(source_compress_dialog_tests PRIVATE Qt6::Widgets Qt6::Test sevenzip_common sevenzip_location)
    target_compile_definitions(source_compress_dialog_tests PRIVATE Z7_EXTERNAL_CODECS)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(source_compress_dialog_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME source_compress_dialog COMMAND source_compress_dialog_tests -silent)
    set_tests_properties(source_compress_dialog PROPERTIES TIMEOUT 30
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/source-compress-dialog.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
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

    set_tests_properties(compression_order PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")

    set_tests_properties(compression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    foreach(suite compression compression_order compression_dictionary compression_memory compression_resources compression_solid compression_limit)
        set_property(TEST ${suite} APPEND PROPERTY ENVIRONMENT "SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/${suite}-settings.ini")
    endforeach()
    qt_add_executable(archive_item_open_tests tests/archive_item_open_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp  src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_rename.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h  src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(archive_item_open_tests PRIVATE src)
    target_link_libraries(archive_item_open_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME archive_item_open COMMAND archive_item_open_tests -silent)
    set_tests_properties(archive_item_open PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/file-viewer-settings.ini")
    qt_add_executable(nested_archive_tests tests/nested_archive_tests.cpp src/CPP/7zip/UI/GUI/CompressDialog.cpp src/CPP/7zip/UI/GUI/CompressDialog.h src/archivebackend.cpp src/archivebackend_validation.cpp src/archivebackend_mutations.cpp src/archivebackend_listing.cpp src/archivebackend.h src/filesystemscanner.cpp src/filesystemscanner.h   src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h src/mainwindow_selection.cpp src/CPP/7zip/UI/FileManager/PanelKey.cpp  src/filetable.cpp src/filetable.h src/mainwindow.h src/CPP/7zip/UI/Agent/Agent.cpp src/CPP/7zip/UI/Agent/Agent.h src/CPP/7zip/UI/Agent/AgentProxy.cpp src/CPP/7zip/UI/Agent/AgentProxy.h src/mainwindow.cpp src/mainwindow_menus.cpp src/mainwindow_panel.cpp src/mainwindow_backendevents.cpp src/mainwindow_archiveopening.cpp src/mainwindow_volumes.cpp src/mainwindow_fileoperations.cpp src/mainwindow_rename.cpp src/mainwindow_archive_transfer.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h src/mainwindow_filelist.cpp src/mainwindow_archivelist.cpp src/mainwindow_preferences.cpp src/mainwindow_dialogs.cpp src/mainwindow_archivecommands.cpp src/createarchivedialog.cpp src/createarchivedialog_encryption.cpp src/createarchivedialog_compression.cpp src/createarchivedialog_resources.cpp src/createarchivedialog_preferences.cpp src/createarchivedialog.h src/extractdialog.h src/combodialog.h src/copydialog.h src/settingsdialog.h  src/CPP/7zip/UI/FileManager/SplitUtils.cpp src/CPP/7zip/UI/FileManager/SplitUtils.h src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp src/CPP/7zip/UI/FileManager/PanelSplitFile.h src/volumeoperations.cpp src/volumeoperations.h src/CPP/7zip/UI/FileManager/SplitDialog.cpp src/CPP/7zip/UI/FileManager/SplitDialog.h)
    target_include_directories(nested_archive_tests PRIVATE src)
    target_link_libraries(nested_archive_tests PRIVATE Qt6::Widgets Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(nested_archive_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME nested_archive COMMAND nested_archive_tests -silent)
    set_tests_properties(nested_archive PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/nested-archive-settings.ini")
    get_target_property(native_navigation_sources nested_archive_tests SOURCES)
    list(REMOVE_ITEM native_navigation_sources tests/nested_archive_tests.cpp)
    qt_add_executable(native_navigation_window_tests tests/native_navigation_window_tests.cpp
        tests/layout_inspection.cpp tests/layout_inspection.h
        ${native_navigation_sources})
    target_include_directories(native_navigation_window_tests PRIVATE src)
    target_link_libraries(native_navigation_window_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME native_navigation_window COMMAND native_navigation_window_tests -silent)
    set_tests_properties(native_navigation_window PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native-navigation-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(file_transfer_tests tests/file_transfer_tests.cpp src/CPP/7zip/UI/FileManager/FSFolder.h src/filetransferoperation.cpp src/filetransferoperation.h src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h)
    target_include_directories(file_transfer_tests PRIVATE src)
    target_link_libraries(file_transfer_tests PRIVATE Qt6::Test sevenzip_filesystem)
    add_test(NAME file_transfer COMMAND file_transfer_tests -silent)
    qt_add_executable(file_transfer_direct_tests tests/file_transfer_direct_tests.cpp
        src/filetransferoperation.cpp src/filetransferoperation.h src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h)
    target_include_directories(file_transfer_direct_tests PRIVATE src)
    target_link_libraries(file_transfer_direct_tests PRIVATE Qt6::Test sevenzip_filesystem)
    add_test(NAME file_transfer_direct COMMAND file_transfer_direct_tests -silent)
    qt_add_executable(language_file_transfer_tests tests/language_file_transfer_tests.cpp
        src/filetransferoperation.cpp src/filetransferoperation.h src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h)
    target_include_directories(language_file_transfer_tests PRIVATE src)
    target_link_libraries(language_file_transfer_tests PRIVATE Qt6::Test sevenzip_filesystem)
    add_test(NAME language_file_transfer COMMAND language_file_transfer_tests -silent)
    set_tests_properties(language_file_transfer PROPERTIES TIMEOUT 60)
    qt_add_executable(fs_folder_copy_tests tests/fs_folder_copy_tests.cpp)
    target_include_directories(fs_folder_copy_tests PRIVATE src)
    target_link_libraries(fs_folder_copy_tests PRIVATE sevenzip_filesystem Qt6::Test)
    add_test(NAME fs_folder_copy COMMAND fs_folder_copy_tests -silent)
    qt_add_executable(filesystem_link_tests tests/filesystem_link_tests.cpp
        src/filetransferoperation.cpp src/filetransferoperation.h src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h)
    target_link_libraries(filesystem_link_tests PRIVATE sevenzip_filesystem Qt6::Test)
    add_test(NAME filesystem_link COMMAND filesystem_link_tests -silent)
    set_tests_properties(filesystem_link PROPERTIES TIMEOUT 60)
    if(WIN32)
        qt_add_executable(alt_streams_folder_tests tests/alt_streams_folder_tests.cpp)
        target_link_libraries(alt_streams_folder_tests PRIVATE sevenzip_panel Qt6::Test)
        add_test(NAME alt_streams_folder COMMAND alt_streams_folder_tests -silent)
        set_tests_properties(alt_streams_folder PROPERTIES TIMEOUT 60)
    endif()
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
    qt_add_executable(window_placement_tests tests/window_placement_tests.cpp
        tests/layout_inspection.cpp src/mainwindow_selectionpaths.cpp ${panel_sort_sources})
    target_include_directories(window_placement_tests PRIVATE src tests)
    target_link_libraries(window_placement_tests PRIVATE sevenzip_panel sevenzip_filesystem
        sevenzip_location sevenzip_folder_loader Qt6::Test)
    add_test(NAME window_placement COMMAND window_placement_tests -silent)
    set_tests_properties(window_placement PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/window-placement-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(app_lifecycle_tests tests/app_lifecycle_tests.cpp)
    qt_add_executable(app_copy_routing_tests tests/app_copy_routing_tests.cpp
        tests/layout_inspection.cpp)
    target_include_directories(app_copy_routing_tests PRIVATE src tests)
    target_link_libraries(app_copy_routing_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME app_copy_routing COMMAND app_copy_routing_tests -silent)
    set_tests_properties(app_copy_routing PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
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
    qt_add_executable(panel_column_schema_tests tests/panel_column_schema_tests.cpp tests/schema_folder.cpp tests/panel_load_folder.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_column_schema_tests PRIVATE src tests)
    target_link_libraries(panel_column_schema_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_column_schema_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_column_schema COMMAND panel_column_schema_tests -silent)
    set_tests_properties(panel_column_schema PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-column-schema.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_column_changes_tests tests/panel_column_changes_tests.cpp tests/schema_folder.cpp tests/panel_load_folder.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_column_changes_tests PRIVATE src tests)
    target_link_libraries(panel_column_changes_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_column_changes_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_column_changes COMMAND panel_column_changes_tests -silent)
    set_tests_properties(panel_column_changes PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-column-changes.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(native_column_workflow_tests tests/native_column_workflow_tests.cpp tests/schema_folder.cpp tests/panel_load_folder.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(native_column_workflow_tests PRIVATE src tests)
    target_link_libraries(native_column_workflow_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(native_column_workflow_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(native_column_workflow_tests PRIVATE "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME native_column_workflow COMMAND native_column_workflow_tests -silent)
    set_tests_properties(native_column_workflow PROPERTIES TIMEOUT 90 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native-column-workflow.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(archive_reload_command_tests tests/archive_reload_command_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(archive_reload_command_tests PRIVATE src tests)
    target_link_libraries(archive_reload_command_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(archive_reload_command_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(archive_reload_command_tests PRIVATE "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME archive_reload_command COMMAND archive_reload_command_tests -silent)
    set_tests_properties(archive_reload_command PROPERTIES TIMEOUT 120 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/archive-reload-command.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(native_renderer_failure_tests tests/native_renderer_failure_tests.cpp tests/native_property_failure_runtime.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    qt_add_executable(native_parent_publication_tests tests/native_parent_publication_tests.cpp tests/native_property_failure_runtime.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(native_parent_publication_tests PRIVATE src tests)
    target_link_libraries(native_parent_publication_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(native_parent_publication_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    target_compile_definitions(native_parent_publication_tests PRIVATE
        "SEVENZIP_EXTERNAL_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_EXTERNAL_FORMAT_FIXTURE=\"$<TARGET_FILE:external_format_fixture>\"")
    add_dependencies(native_parent_publication_tests external_format_fixture)
    add_test(NAME native_parent_publication COMMAND native_parent_publication_tests -silent)
    set_tests_properties(native_parent_publication PROPERTIES TIMEOUT 120 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native-parent.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(native_drop_publication_tests tests/native_drop_publication_tests.cpp tests/native_property_failure_runtime.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(native_drop_publication_tests PRIVATE src tests)
    target_link_libraries(native_drop_publication_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(native_drop_publication_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    target_compile_definitions(native_drop_publication_tests PRIVATE
        "SEVENZIP_EXTERNAL_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_EXTERNAL_FORMAT_FIXTURE=\"$<TARGET_FILE:external_format_fixture>\"")
    add_dependencies(native_drop_publication_tests external_format_fixture)
    add_test(NAME native_drop_publication COMMAND native_drop_publication_tests -silent)
    set_tests_properties(native_drop_publication PROPERTIES TIMEOUT 120 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native-drop.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(native_rename_publication_tests tests/native_rename_publication_tests.cpp tests/native_property_failure_runtime.cpp tests/external_editor_fixture.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(native_rename_publication_tests PRIVATE src tests)
    target_link_libraries(native_rename_publication_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(native_rename_publication_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    target_compile_definitions(native_rename_publication_tests PRIVATE
        "SEVENZIP_EXTERNAL_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_EXTERNAL_FORMAT_FIXTURE=\"$<TARGET_FILE:external_format_fixture>\"")
    add_dependencies(native_rename_publication_tests external_format_fixture)
    foreach(rename_rows 2 301)
        add_test(NAME native_rename_${rename_rows} COMMAND native_rename_publication_tests -silent)
        set_tests_properties(native_rename_${rename_rows} PROPERTIES TIMEOUT 360 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_RENAME_ROWS=${rename_rows};SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native-rename-${rename_rows}.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    endforeach()
    qt_add_executable(native_external_publication_tests tests/native_external_publication_tests.cpp tests/native_property_failure_runtime.cpp tests/external_editor_fixture.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(native_external_publication_tests PRIVATE src tests)
    target_link_libraries(native_external_publication_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(native_external_publication_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    target_compile_definitions(native_external_publication_tests PRIVATE
        "SEVENZIP_EXTERNAL_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_EXTERNAL_FORMAT_FIXTURE=\"$<TARGET_FILE:external_format_fixture>\"")
    add_dependencies(native_external_publication_tests external_format_fixture)
    foreach(external_rows 2 301)
        add_test(NAME native_external_${external_rows} COMMAND native_external_publication_tests -silent)
        set_tests_properties(native_external_${external_rows} PROPERTIES TIMEOUT 240 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_EXTERNAL_ROWS=${external_rows};SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native-external-${external_rows}.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    endforeach()
    qt_add_executable(native_nested_publication_tests tests/native_nested_publication_tests.cpp tests/native_property_failure_runtime.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(native_nested_publication_tests PRIVATE src tests)
    target_link_libraries(native_nested_publication_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(native_nested_publication_tests PRIVATE src/mainwindow_selectionpaths.cpp
        tests/fixtures/widget_dialogs.cpp)
    target_compile_definitions(native_nested_publication_tests PRIVATE
        "SEVENZIP_EXTERNAL_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_EXTERNAL_FORMAT_FIXTURE=\"$<TARGET_FILE:external_format_fixture>\"")
    add_dependencies(native_nested_publication_tests external_format_fixture)
    foreach(nested_route stream memory disk)
        add_test(NAME native_nested_${nested_route} COMMAND native_nested_publication_tests -silent)
        set_tests_properties(native_nested_${nested_route} PROPERTIES TIMEOUT 180 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_NESTED_ROUTE=${nested_route};SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native-nested-${nested_route}.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    endforeach()
    target_include_directories(native_renderer_failure_tests PRIVATE src tests)
    target_link_libraries(native_renderer_failure_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(native_renderer_failure_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    target_compile_definitions(native_renderer_failure_tests PRIVATE
        "SEVENZIP_EXTERNAL_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_EXTERNAL_FORMAT_FIXTURE=\"$<TARGET_FILE:external_format_fixture>\"")
    add_dependencies(native_renderer_failure_tests external_format_fixture)
    add_test(NAME native_renderer_failure COMMAND native_renderer_failure_tests -silent)
    set_tests_properties(native_renderer_failure PROPERTIES TIMEOUT 180 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native-renderer-failure.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(native_renderer_copy_tests tests/native_renderer_copy_tests.cpp tests/native_property_failure_runtime.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(native_renderer_copy_tests PRIVATE src tests)
    target_link_libraries(native_renderer_copy_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(native_renderer_copy_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    target_compile_definitions(native_renderer_copy_tests PRIVATE
        "SEVENZIP_EXTERNAL_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_EXTERNAL_FORMAT_FIXTURE=\"$<TARGET_FILE:external_format_fixture>\"")
    add_dependencies(native_renderer_copy_tests external_format_fixture)
    add_test(NAME native_renderer_copy COMMAND native_renderer_copy_tests -silent)
    set_tests_properties(native_renderer_copy PROPERTIES TIMEOUT 180 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native-renderer-copy.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_creation_path_tests tests/panel_creation_path_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_creation_path_tests PRIVATE src tests)
    target_link_libraries(panel_creation_path_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_creation_path_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_creation_path COMMAND panel_creation_path_tests -silent)
    set_tests_properties(panel_creation_path PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-creation-path.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_creation_publication_tests tests/panel_creation_publication_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_creation_publication_tests PRIVATE src tests)
    target_link_libraries(panel_creation_publication_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_creation_publication_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(panel_creation_publication_tests PRIVATE "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME panel_creation_publication COMMAND panel_creation_publication_tests -silent)
    set_tests_properties(panel_creation_publication PROPERTIES TIMEOUT 120 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-creation-publication.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(interarchive_recovery_tests tests/interarchive_recovery_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(interarchive_recovery_tests PRIVATE src tests)
    target_link_libraries(interarchive_recovery_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(interarchive_recovery_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(interarchive_recovery_tests PRIVATE "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME interarchive_recovery COMMAND interarchive_recovery_tests -silent)
    set_tests_properties(interarchive_recovery PROPERTIES TIMEOUT 120 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/interarchive-recovery.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(interarchive_refresh_tests tests/interarchive_refresh_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(interarchive_refresh_tests PRIVATE src tests)
    target_link_libraries(interarchive_refresh_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(interarchive_refresh_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(interarchive_refresh_tests PRIVATE "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME interarchive_refresh COMMAND interarchive_refresh_tests -silent)
    set_tests_properties(interarchive_refresh PROPERTIES TIMEOUT 120 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/interarchive-refresh.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(copy_archive_refresh_tests tests/copy_archive_refresh_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(copy_archive_refresh_tests PRIVATE src tests)
    target_link_libraries(copy_archive_refresh_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(copy_archive_refresh_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(copy_archive_refresh_tests PRIVATE "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME copy_archive_refresh COMMAND copy_archive_refresh_tests -silent)
    set_tests_properties(copy_archive_refresh PROPERTIES TIMEOUT 120 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/copy-archive-refresh.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(drag_reload_completion_tests tests/drag_reload_completion_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(drag_reload_completion_tests PRIVATE src tests)
    target_link_libraries(drag_reload_completion_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(drag_reload_completion_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME drag_reload_completion COMMAND drag_reload_completion_tests -silent)
    set_tests_properties(drag_reload_completion PROPERTIES TIMEOUT 120 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/drag-reload-completion.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_load_count_tests tests/panel_load_count_tests.cpp tests/panel_load_folder.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_load_count_tests PRIVATE src tests)
    target_link_libraries(panel_load_count_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_load_count_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_load_count COMMAND panel_load_count_tests -silent)
    set_tests_properties(panel_load_count PROPERTIES TIMEOUT 120 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-load-count.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_activation_tests tests/panel_activation_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_activation_tests PRIVATE src tests)
    target_link_libraries(panel_activation_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_activation_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(panel_activation_tests PRIVATE "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME panel_activation COMMAND panel_activation_tests -silent)
    set_tests_properties(panel_activation PROPERTIES TIMEOUT 180 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-activation.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_item_changed_tests tests/panel_item_changed_tests.cpp tests/panel_load_folder.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_item_changed_tests PRIVATE src tests)
    target_link_libraries(panel_item_changed_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_item_changed_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_item_changed COMMAND panel_item_changed_tests -silent)
    set_tests_properties(panel_item_changed PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-item-changed.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_cell_text_tests tests/panel_cell_text_tests.cpp tests/schema_folder.cpp tests/panel_load_folder.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_cell_text_tests PRIVATE src tests)
    target_link_libraries(panel_cell_text_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_cell_text_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_cell_text COMMAND panel_cell_text_tests -silent)
    set_tests_properties(panel_cell_text PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-cell-text.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_column_sort_tests tests/panel_column_sort_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_column_sort_tests PRIVATE src tests)
    target_link_libraries(panel_column_sort_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_column_sort_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_column_sort COMMAND panel_column_sort_tests -silent)
    set_tests_properties(panel_column_sort PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-column-sort.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_column_menu_tests tests/panel_column_menu_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_column_menu_tests PRIVATE src tests)
    target_link_libraries(panel_column_menu_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_column_menu_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_column_menu COMMAND panel_column_menu_tests -silent)
    set_tests_properties(panel_column_menu PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-column-menu.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_column_initialization_tests tests/panel_column_initialization_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_column_initialization_tests PRIVATE src tests)
    target_link_libraries(panel_column_initialization_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_column_initialization_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_column_initialization COMMAND panel_column_initialization_tests -silent)
    set_tests_properties(panel_column_initialization PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-column-initialization.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_list_load_tests tests/panel_list_load_tests.cpp tests/panel_load_folder.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_list_load_tests PRIVATE src tests)
    target_link_libraries(panel_list_load_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_list_load_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_list_load COMMAND panel_list_load_tests -silent)
    qt_add_executable(navigation_reload_tests tests/navigation_reload_tests.cpp tests/panel_load_folder.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(navigation_reload_tests PRIVATE src tests)
    target_link_libraries(navigation_reload_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(navigation_reload_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME navigation_reload COMMAND navigation_reload_tests -silent)
    qt_add_executable(panel_deleted_draw_tests tests/panel_deleted_draw_tests.cpp tests/deleted_folder.cpp tests/panel_load_folder.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_deleted_draw_tests PRIVATE src tests)
    target_link_libraries(panel_deleted_draw_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_deleted_draw_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_deleted_draw COMMAND panel_deleted_draw_tests -silent)
    set_tests_properties(panel_deleted_draw PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-deleted-draw.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(navigation_reload PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/navigation-reload.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_item_icons_tests tests/panel_item_icons_tests.cpp tests/panel_load_folder.cpp tests/layout_inspection.cpp tests/external_editor_fixture.cpp ${panel_sort_sources})
    target_include_directories(panel_item_icons_tests PRIVATE src tests)
    target_link_libraries(panel_item_icons_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_item_icons_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_item_icons COMMAND panel_item_icons_tests -silent)
    qt_add_executable(panel_name_text_tests tests/panel_name_text_tests.cpp tests/panel_load_folder.cpp tests/layout_inspection.cpp tests/external_editor_fixture.cpp ${panel_sort_sources})
    target_include_directories(panel_name_text_tests PRIVATE src tests)
    target_link_libraries(panel_name_text_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_name_text_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_name_text COMMAND panel_name_text_tests -silent)
    qt_add_executable(archive_rename_window_tests tests/archive_rename_window_tests.cpp tests/layout_inspection.cpp tests/external_editor_fixture.cpp ${panel_sort_sources})
    target_include_directories(archive_rename_window_tests PRIVATE src tests)
    target_link_libraries(archive_rename_window_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(archive_rename_window_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME archive_rename_window COMMAND archive_rename_window_tests -silent)
    set_tests_properties(archive_rename_window PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/archive-rename-window.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_rename_state_tests tests/panel_rename_state_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_rename_state_tests PRIVATE src tests)
    target_link_libraries(panel_rename_state_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_rename_state_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_rename_state COMMAND panel_rename_state_tests -silent)
    set_tests_properties(panel_rename_state PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-rename-state.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(deferred_rename_tests tests/deferred_rename_tests.cpp tests/external_editor_fixture.cpp ${panel_sort_sources})
    target_include_directories(deferred_rename_tests PRIVATE src tests)
    target_link_libraries(deferred_rename_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(deferred_rename_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME deferred_rename COMMAND deferred_rename_tests -silent)
    set_tests_properties(deferred_rename PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/deferred-rename.ini")
    qt_add_executable(panel_refresh_names_tests tests/panel_refresh_names_tests.cpp tests/panel_load_folder.cpp ${panel_sort_sources})
    target_include_directories(panel_refresh_names_tests PRIVATE src tests)
    target_link_libraries(panel_refresh_names_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_refresh_names_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_refresh_names COMMAND panel_refresh_names_tests -silent)
    set_tests_properties(panel_refresh_names PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(panel_refresh_notifications_tests tests/panel_refresh_notifications_tests.cpp tests/panel_load_folder.cpp ${panel_sort_sources})
    target_include_directories(panel_refresh_notifications_tests PRIVATE src tests)
    target_link_libraries(panel_refresh_notifications_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_refresh_notifications_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_refresh_notifications COMMAND panel_refresh_notifications_tests -silent)
    set_tests_properties(panel_refresh_notifications PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(panel_filesystem_sort_tests tests/panel_filesystem_sort_tests.cpp ${panel_sort_sources})
    target_include_directories(panel_filesystem_sort_tests PRIVATE src tests)
    target_link_libraries(panel_filesystem_sort_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_filesystem_sort_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_filesystem_sort COMMAND panel_filesystem_sort_tests -silent)
    set_tests_properties(panel_filesystem_sort PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    qt_add_executable(panel_archive_sort_tests tests/panel_archive_sort_tests.cpp ${panel_sort_sources})
    target_include_directories(panel_archive_sort_tests PRIVATE src tests)
    target_link_libraries(panel_archive_sort_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_archive_sort_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_archive_sort COMMAND panel_archive_sort_tests -silent)
    set_tests_properties(panel_archive_sort PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(panel_sort_commands_tests tests/panel_sort_commands_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_sort_commands_tests PRIVATE src tests)
    target_link_libraries(panel_sort_commands_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_sort_commands_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_sort_commands COMMAND panel_sort_commands_tests -silent)
    set_tests_properties(panel_sort_commands PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-sort-commands.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_refresh_drop_tests tests/panel_refresh_drop_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_refresh_drop_tests PRIVATE src tests)
    target_link_libraries(panel_refresh_drop_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_refresh_drop_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_refresh_drop COMMAND panel_refresh_drop_tests -silent)
    set_tests_properties(panel_refresh_drop PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-refresh-drop.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_refresh_completion_tests tests/panel_refresh_completion_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_refresh_completion_tests PRIVATE src tests)
    target_link_libraries(panel_refresh_completion_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_refresh_completion_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_refresh_completion COMMAND panel_refresh_completion_tests -silent)
    set_tests_properties(panel_refresh_completion PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-refresh-completion.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(panel_name_text PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-name-text-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(panel_item_icons PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-item-icons-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(panel_list_load PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-list-load-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_copy_reload_tests tests/panel_copy_reload_tests.cpp tests/panel_load_folder.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    qt_add_executable(copy_publication_order_tests tests/copy_publication_order_tests.cpp ${panel_sort_sources})
    target_include_directories(copy_publication_order_tests PRIVATE src tests)
    target_link_libraries(copy_publication_order_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(copy_publication_order_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME copy_publication_order COMMAND copy_publication_order_tests -silent)
    set_tests_properties(copy_publication_order PROPERTIES TIMEOUT 45 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/copy-publication-order-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    target_include_directories(panel_copy_reload_tests PRIVATE src tests)
    target_link_libraries(panel_copy_reload_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_copy_reload_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_copy_reload COMMAND panel_copy_reload_tests -silent)
    set_tests_properties(panel_copy_reload PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-copy-reload-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(filesystem_reload_tests tests/filesystem_reload_tests.cpp ${panel_sort_sources})
    target_include_directories(filesystem_reload_tests PRIVATE src tests)
    target_link_libraries(filesystem_reload_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(filesystem_reload_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME filesystem_reload COMMAND filesystem_reload_tests -silent)
    set_tests_properties(filesystem_reload PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/filesystem-reload-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(address_navigation_tests tests/address_navigation_tests.cpp ${panel_sort_sources})
    target_include_directories(address_navigation_tests PRIVATE src tests)
    target_link_libraries(address_navigation_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(address_navigation_tests PRIVATE src/mainwindow_selectionpaths.cpp tests/external_editor_fixture.cpp)
    add_test(NAME address_navigation COMMAND address_navigation_tests -silent)
    set_tests_properties(address_navigation PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/address-navigation-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(panel_columns PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-columns-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_parent_reload_tests tests/panel_parent_reload_tests.cpp tests/layout_inspection.cpp ${panel_sort_sources})
    target_include_directories(panel_parent_reload_tests PRIVATE src tests)
    target_link_libraries(panel_parent_reload_tests PRIVATE sevenzip_location sevenzip_folder_loader Qt6::Test)
    target_sources(panel_parent_reload_tests PRIVATE src/mainwindow_selectionpaths.cpp)
    add_test(NAME panel_parent_reload COMMAND panel_parent_reload_tests -silent)
    set_tests_properties(panel_parent_reload PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-parent-reload-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
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
    set_tests_properties(panel_key PROPERTIES TIMEOUT 90 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-key-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(panel_sort PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-sort-settings.ini;SEVENZIP_SCREENSHOT_DIR=${CMAKE_BINARY_DIR}/ui-screenshots")
    qt_add_executable(panel_selection_tests tests/panel_selection_tests.cpp
        src/CPP/7zip/UI/FileManager/PanelSelect.cpp src/CPP/7zip/UI/FileManager/PanelSelect.h
        )
    target_include_directories(panel_selection_tests PRIVATE src)
    target_link_libraries(panel_selection_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME panel_selection COMMAND panel_selection_tests -silent)
    set_tests_properties(panel_selection PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(wildcard_path_tests tests/wildcard_path_tests.cpp)
    target_include_directories(wildcard_path_tests PRIVATE src)
    target_link_libraries(wildcard_path_tests PRIVATE sevenzip_common Qt6::Test)
    add_test(NAME wildcard_path COMMAND wildcard_path_tests -silent)
    set_tests_properties(wildcard_path PROPERTIES TIMEOUT 30)
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
    foreach(target IN ITEMS ui_layout_tests compression_tests)
        target_sources(${target} PRIVATE tests/layout_inspection.cpp)
    endforeach()
    target_sources(properties_tests PRIVATE tests/layout_inspection.cpp)
    qt_add_executable(fm_settings_tests tests/fm_settings_tests.cpp tests/layout_inspection.cpp)
    qt_add_executable(memory_lock_tests tests/memory_lock_tests.cpp)
    target_include_directories(memory_lock_tests PRIVATE src tests)
    target_link_libraries(memory_lock_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME memory_lock COMMAND memory_lock_tests -silent)
    set_tests_properties(memory_lock PROPERTIES TIMEOUT 30)
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

foreach(target IN ITEMS sevenzip_desktop archive_proxy_tests archive_backend_tests archive_mutation_tests ui_layout_tests ui_state_tests ui_operations_tests ui_copy_same_tests ui_copy_routing_tests ui_update_modes_tests ui_archive_volumes_tests external_editors_ui_tests verctrl_tests nested_edit_chain_tests archive_external_edit_tests archive_external_security_tests ui_navigation_tests ui_archive_states_tests flat_view_tests settings_tests properties_tests checksum_tests extraction_options_tests compression_tests archive_item_open_tests nested_archive_tests volume_operations_tests)
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
            src/mainwindow_columns.cpp src/mainwindow_refresh.cpp
            src/mainwindow_panelkey.cpp src/qtpanelkey.h
            src/mainwindow_shortcuts.cpp
            src/mainwindow_close.cpp src/qtfilemanagerwindowservices.h
            src/mainwindow_startup.cpp src/qtfilemanagerinitservices.h
            src/qtfilemanagersplitter.cpp src/qtfilemanagersplitter.h
            src/qtfilemanagercommands.cpp src/qtfilemanagercommands.h
            src/qtfilemanagercommandworkflows.cpp src/mainwindow_root.cpp
            src/qtfilemanagershellmenu.cpp src/qtfilemanagershellmenu.h
            src/qtnativeshellmenu.cpp src/qtnativeshellmenu.h
            src/qtnativeshellmenupresentation.cpp src/qtnativeshellmenupresentation.h
            src/qtprogramfilemenu.cpp src/qtprogramfilemenu.h
            src/qtfilemanagerdrag.cpp src/qtfilemanagerdrag.h src/qtfilemanagerdragpresentation.cpp
            src/qtfilemanagerdraginput.cpp
            src/mainwindow_twopanel.cpp
            src/qtfilemenuprojection.cpp src/qtfilemenuprojection.h)
    endif()
    if("src/archivebackend.cpp" IN_LIST engine_consumer_sources)
        target_sources(${engine_consumer} PRIVATE src/archivebackend_sourceupdate.cpp src/archivebackend_sourceplan.cpp
            src/archivebackend_process.cpp
            src/archivebackend_sourceextract.cpp src/archivebackend_volumes.cpp
            src/archivebackend_creation.cpp src/archivebackend_hash.cpp src/archivebackend_preparation.cpp
            src/archivebackend_workerprogress.cpp src/archivebackend_transfer.cpp
            src/archivebackend_completion.cpp src/archivebackend_compression.cpp
            src/qtarchivetransferbackup.h)
        target_link_libraries(${engine_consumer} PRIVATE sevenzip_filesystem)
    endif()
    if("src/createarchivedialog.cpp" IN_LIST engine_consumer_sources)
        target_link_libraries(${engine_consumer} PRIVATE sevenzip_panel)
        target_sources(${engine_consumer} PRIVATE src/createarchivedialog_language.cpp)
        target_sources(${engine_consumer} PRIVATE src/createarchivedialog_volumes.cpp
            src/createarchivedialog_advanced.cpp src/createarchivedialog_formats.cpp
            src/createarchivedialog_sfx.cpp src/createarchivedialog_source.cpp)
    endif()
endforeach()

# Window runtime uses the authoritative source-owned archive location factory.
if(TARGET flat_view_tests)
    set_property(TEST flat_view APPEND PROPERTY ENVIRONMENT
        "SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
endif()
if(TARGET ui_navigation_tests)
    target_sources(ui_navigation_tests PRIVATE tests/layout_inspection.cpp)
    set_property(TEST ui_navigation APPEND PROPERTY ENVIRONMENT
        "SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
endif()
foreach(target IN ITEMS sevenzip_desktop ui_layout_tests ui_state_tests ui_operations_tests ui_copy_same_tests ui_copy_routing_tests ui_update_modes_tests ui_archive_volumes_tests external_editors_ui_tests verctrl_tests nested_edit_chain_tests archive_external_edit_tests archive_external_security_tests
    ui_navigation_tests ui_archive_states_tests flat_view_tests settings_tests properties_tests
    checksum_tests archive_item_open_tests nested_archive_tests native_navigation_window_tests panel_sort_tests combine_name_tests
    panel_key_tests main_shell_tests panel_shell_tests volume_workflow_tests bookmark_tests panel_focus_tests panel_selected_state_tests panel_runtime_tests properties_runtime_tests archive_selection_runtime_tests filesystem_selection_runtime_tests binding_lifecycle_tests app_lifecycle_window_tests)
    if(TARGET ${target})
        target_link_libraries(${target} PRIVATE sevenzip_location sevenzip_folder_loader)
        target_sources(${target} PRIVATE src/mainwindow_selectionpaths.cpp)
    endif()
endforeach()

foreach(target IN ITEMS sevenzip_desktop ui_layout_tests ui_state_tests ui_operations_tests ui_copy_same_tests ui_copy_routing_tests ui_update_modes_tests ui_archive_volumes_tests external_editors_ui_tests verctrl_tests nested_edit_chain_tests archive_external_edit_tests archive_external_security_tests
    ui_navigation_tests ui_archive_states_tests flat_view_tests settings_tests properties_tests
    checksum_tests archive_item_open_tests nested_archive_tests native_navigation_window_tests panel_sort_tests combine_name_tests panel_key_tests main_shell_tests panel_shell_tests volume_workflow_tests)
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
        tests/layout_inspection.cpp ${source_properties_sources})
    target_include_directories(source_properties_tests PRIVATE src tests)
    target_link_libraries(source_properties_tests PRIVATE ${source_properties_libraries})
    add_test(NAME source_properties COMMAND source_properties_tests -silent)
    set_tests_properties(source_properties PROPERTIES TIMEOUT 90
        RESOURCE_LOCK sevenzip_ui_artifacts ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/source_properties.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(agent_property_projection_tests tests/agent_property_projection_tests.cpp
        ${source_properties_sources})
    target_include_directories(agent_property_projection_tests PRIVATE src tests)
    target_link_libraries(agent_property_projection_tests PRIVATE ${source_properties_libraries})
    add_test(NAME agent_property_projection COMMAND agent_property_projection_tests -silent)
    set_tests_properties(agent_property_projection PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/agent-property-projection.ini")
endif()

if(BUILD_TESTING)
    qt_add_executable(language_tests tests/language_tests.cpp tests/layout_inspection.cpp)
    qt_add_executable(native_language_host_tests tests/native_language_host_tests.cpp)
    target_include_directories(native_language_host_tests PRIVATE src)
    target_link_libraries(native_language_host_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME native_language_host COMMAND native_language_host_tests -silent)
    get_target_property(language_backend_sources archive_backend_tests SOURCES)
    list(REMOVE_ITEM language_backend_sources tests/archive_backend_tests.cpp)
    get_target_property(language_backend_libraries archive_backend_tests LINK_LIBRARIES)
    qt_add_executable(language_source_update_tests tests/language_source_update_tests.cpp
        ${language_backend_sources})
    target_include_directories(language_source_update_tests PRIVATE src)
    target_link_libraries(language_source_update_tests PRIVATE ${language_backend_libraries})
    add_test(NAME language_source_update COMMAND language_source_update_tests -silent)
    set_tests_properties(language_source_update PROPERTIES TIMEOUT 60)
    target_include_directories(language_tests PRIVATE src src/CPP/7zip/UI/Common tests)
    target_link_libraries(language_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME language COMMAND language_tests -silent)
    set_tests_properties(language PROPERTIES TIMEOUT 60 RESOURCE_LOCK sevenzip_ui_artifacts
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(menu_page_tests tests/menu_page_tests.cpp tests/layout_inspection.cpp)
    qt_add_executable(shell_context_tests tests/shell_context_tests.cpp)
    target_include_directories(shell_context_tests PRIVATE src)
    target_link_libraries(shell_context_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME shell_context COMMAND shell_context_tests -silent)
    if(WIN32)
        qt_add_executable(native_system_menu_tests tests/native_system_menu_tests.cpp)
        qt_add_executable(native_menu_integration_tests tests/native_menu_integration_tests.cpp)
        target_include_directories(native_menu_integration_tests PRIVATE src)
        target_link_libraries(native_menu_integration_tests PRIVATE sevenzip_panel sevenzip_location Qt6::Test)
        add_dependencies(native_menu_integration_tests sevenzip_shell)
        if("$ENV{COMPUTERNAME}" STREQUAL "RUNNER-WINDOWS")
            add_test(NAME native_menu_integration COMMAND native_menu_integration_tests -silent)
            set_tests_properties(native_menu_integration PROPERTIES TIMEOUT 60
                RESOURCE_LOCK sevenzip_shell_registration ENVIRONMENT "QT_QPA_PLATFORM=windows")
        endif()
        qt_add_executable(native_menu_presentation_tests tests/native_menu_presentation_tests.cpp
            tests/fixtures/native_menu_provider.cpp tests/fixtures/native_menu_provider.h
            src/qtnativeshellmenupresentation.cpp)
        target_include_directories(native_menu_presentation_tests PRIVATE src)
        target_link_libraries(native_menu_presentation_tests PRIVATE sevenzip_panel Qt6::Test)
        add_test(NAME native_menu_presentation COMMAND native_menu_presentation_tests -silent)
        if(EXISTS "${CMAKE_SOURCE_DIR}/build/windows-reference/reference.json")
            qt_add_executable(native_rendered_system_menu_tests
                tests/native_rendered_system_menu_tests.cpp src/qtnativeshellmenupresentation.cpp)
            target_include_directories(native_rendered_system_menu_tests PRIVATE src)
            target_link_libraries(native_rendered_system_menu_tests PRIVATE sevenzip_panel Qt6::Test advapi32)
            target_compile_definitions(native_rendered_system_menu_tests PRIVATE
                "SEVENZIP_NATIVE_REFERENCE_DIR=\"${CMAKE_SOURCE_DIR}/build/windows-reference\""
                "SEVENZIP_UPSTREAM_REVISION=\"${sfx_upstream_revision}\"")
            add_test(NAME native_rendered_system_menu COMMAND native_rendered_system_menu_tests -silent)
            set_tests_properties(native_rendered_system_menu PROPERTIES TIMEOUT 60 ENVIRONMENT
                "QT_QPA_PLATFORM=windows;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
        endif()
        qt_add_executable(native_shell_data_tests tests/native_shell_data_tests.cpp)
        target_sources(native_shell_data_tests PRIVATE
            "${CMAKE_CURRENT_BINARY_DIR}/native_context_menu.rc")
        target_include_directories(native_shell_data_tests PRIVATE src)
        target_link_libraries(native_shell_data_tests PRIVATE sevenzip_panel Qt6::Test)
        add_test(NAME native_shell_data COMMAND native_shell_data_tests -silent)
        qt_add_executable(native_archive_command_line_tests tests/native_archive_command_line_tests.cpp)
        target_include_directories(native_archive_command_line_tests PRIVATE src
            src/CPP/7zip/UI/Common src/CPP/Common)
        target_link_libraries(native_archive_command_line_tests PRIVATE sevenzip_panel Qt6::Test)
        add_test(NAME native_archive_command_line COMMAND native_archive_command_line_tests -silent)
        target_compile_definitions(native_archive_command_line_tests PRIVATE
            "NATIVE_GUI_EXECUTABLE=\"$<TARGET_FILE_DIR:sevenzip_desktop>/7zG.exe\"")
        add_dependencies(native_archive_command_line_tests sevenzip_desktop)
        qt_add_executable(native_gui_workflow_tests tests/native_gui_workflow_tests.cpp)
        target_include_directories(native_gui_workflow_tests PRIVATE src tests)
        target_link_libraries(native_gui_workflow_tests PRIVATE sevenzip_panel Qt6::Test)
        target_compile_definitions(native_gui_workflow_tests PRIVATE
            "NATIVE_GUI_EXECUTABLE=\"$<TARGET_FILE_DIR:sevenzip_desktop>/7zG.exe\"")
        add_dependencies(native_gui_workflow_tests sevenzip_desktop)
        add_test(NAME native_gui_workflow COMMAND native_gui_workflow_tests -silent)
        set_tests_properties(native_gui_workflow PROPERTIES TIMEOUT 180 ENVIRONMENT
            "SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native_gui_workflow.ini"
            RESOURCE_LOCK sevenzip_native_ui)
        qt_add_executable(native_shell_com_tests tests/native_shell_com_tests.cpp)
        add_executable(native_shell_host tests/fixtures/native_shell_host.cpp)
        set_target_properties(native_shell_host PROPERTIES AUTOMOC OFF AUTOUIC OFF)
        target_link_libraries(native_shell_host PRIVATE ole32 shell32 uuid)
        target_include_directories(native_shell_com_tests PRIVATE src)
        target_link_libraries(native_shell_com_tests PRIVATE sevenzip_panel Qt6::Test)
        target_compile_definitions(native_shell_com_tests PRIVATE
            "NATIVE_SHELL_LIBRARY=\"$<TARGET_FILE:sevenzip_shell>\""
            "NATIVE_SHELL_HOST=\"$<TARGET_FILE:native_shell_host>\"")
        add_dependencies(native_shell_com_tests sevenzip_shell native_shell_host)
        add_test(NAME native_shell_com COMMAND native_shell_com_tests -silent)
        qt_add_executable(native_shell_launch_tests tests/native_shell_launch_tests.cpp)
        add_executable(native_shell_launch_host tests/fixtures/native_shell_launch_host.cpp)
        set_target_properties(native_shell_launch_host PROPERTIES AUTOMOC OFF AUTOUIC OFF)
        target_link_libraries(native_shell_launch_host PRIVATE ole32 shell32 uuid advapi32)
        target_include_directories(native_shell_launch_tests PRIVATE tests)
        target_link_libraries(native_shell_launch_tests PRIVATE Qt6::Test advapi32 ole32 oleaut32)
        target_compile_definitions(native_shell_launch_tests PRIVATE
            "NATIVE_SHELL_LIBRARY=\"$<TARGET_FILE:sevenzip_shell>\""
            "NATIVE_SHELL_LAUNCH_HOST=\"$<TARGET_FILE:native_shell_launch_host>\"")
        add_dependencies(native_shell_launch_tests sevenzip_shell native_shell_launch_host sevenzip_desktop)
        add_test(NAME native_shell_launch COMMAND native_shell_launch_tests -silent)
        set_tests_properties(native_shell_launch PROPERTIES TIMEOUT 120 ENVIRONMENT
            "SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native_shell_launch.ini")
        set_tests_properties(native_shell_com PROPERTIES ENVIRONMENT
            "SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native_shell_com.ini")
        target_include_directories(native_system_menu_tests PRIVATE src)
        target_link_libraries(native_system_menu_tests PRIVATE sevenzip_panel Qt6::Test)
        add_test(NAME native_system_menu COMMAND native_system_menu_tests -silent)
        set_tests_properties(native_system_menu PROPERTIES TIMEOUT 30)
    endif()
    qt_add_executable(shell_registry_tests tests/shell_registry_tests.cpp)
    qt_add_executable(menu_policy_tests tests/menu_policy_tests.cpp)
    target_include_directories(menu_policy_tests PRIVATE src)
    target_link_libraries(menu_policy_tests PRIVATE sevenzip_panel sevenzip_location Qt6::Test)
    add_test(NAME menu_policy COMMAND menu_policy_tests -silent)
    set_tests_properties(menu_policy PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    target_include_directories(shell_registry_tests PRIVATE src)
    target_link_libraries(shell_registry_tests PRIVATE sevenzip_panel Qt6::Test)
    add_test(NAME shell_registry COMMAND shell_registry_tests -silent)
    set_tests_properties(shell_registry PROPERTIES TIMEOUT 15)
    set_tests_properties(shell_context PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(system_page_tests tests/system_page_tests.cpp tests/layout_inspection.cpp)
    if(WIN32)
        qt_add_executable(windows_system_registration_tests tests/windows_system_registration_tests.cpp)
        target_include_directories(windows_system_registration_tests PRIVATE src tests)
        target_link_libraries(windows_system_registration_tests PRIVATE sevenzip_location Qt6::Test)
        add_test(NAME windows_system_registration COMMAND windows_system_registration_tests -silent)
        set_tests_properties(windows_system_registration PROPERTIES TIMEOUT 30
            ENVIRONMENT "QT_QPA_PLATFORM=windows")
    endif()
    qt_add_executable(association_services_tests tests/association_services_tests.cpp)
    qt_add_executable(association_file_open_tests tests/association_file_open_tests.cpp)
    if(APPLE)
        qt_add_executable(mac_system_page_tests tests/mac_system_page_tests.cpp tests/layout_inspection.cpp)
        target_include_directories(mac_system_page_tests PRIVATE src tests)
        target_link_libraries(mac_system_page_tests PRIVATE sevenzip_location Qt6::Test)
        set_target_properties(mac_system_page_tests PROPERTIES MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_GUI_IDENTIFIER "org.sevenzip-desktop.tests.system-page")
        add_test(NAME mac_system_page COMMAND mac_system_page_tests -silent)
        set_tests_properties(mac_system_page PROPERTIES TIMEOUT 30
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    endif()
    target_include_directories(association_file_open_tests PRIVATE src)
    target_link_libraries(association_file_open_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME association_file_open COMMAND association_file_open_tests -silent)
    set_tests_properties(association_file_open PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    if(APPLE)
        add_test(NAME macos_association_bundle COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/macos_association_bundle_tests.py
            $<TARGET_BUNDLE_DIR:sevenzip_desktop> ${codec_icon_resource})
    endif()
    target_include_directories(association_services_tests PRIVATE src tests)
    target_link_libraries(association_services_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME association_services COMMAND association_services_tests -silent)
    set_tests_properties(association_services PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        qt_add_executable(linux_system_page_tests tests/linux_system_page_tests.cpp tests/layout_inspection.cpp tests/fixtures/widget_dialogs.cpp)
        target_include_directories(linux_system_page_tests PRIVATE src tests)
        target_link_libraries(linux_system_page_tests PRIVATE sevenzip_location Qt6::Test)
        foreach(language - ru)
            foreach(width 640 520)
                add_test(NAME linux_system_page_${language}_${width}
                    COMMAND linux_system_page_tests "nativeIconsAndPendingCancel:${language}-${width}" -silent)
                set_tests_properties(linux_system_page_${language}_${width} PROPERTIES TIMEOUT 30
                    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
            endforeach()
        endforeach()
        qt_add_executable(linux_association_clear_tests tests/linux_association_clear_tests.cpp)
        target_include_directories(linux_association_clear_tests PRIVATE src)
        target_link_libraries(linux_association_clear_tests PRIVATE sevenzip_location Qt6::Test)
        foreach(other 0 1)
            add_test(NAME linux_association_clear_${other} COMMAND linux_association_clear_tests -silent)
            set_tests_properties(linux_association_clear_${other} PROPERTIES TIMEOUT 15
                ENVIRONMENT "SEVENZIP_ASSOC_OTHER=${other}")
        endforeach()
    endif()
    target_include_directories(system_page_tests PRIVATE src tests)
    target_link_libraries(system_page_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME system_page COMMAND system_page_tests -silent)
    set_tests_properties(system_page PROPERTIES TIMEOUT 60 RESOURCE_LOCK sevenzip_ui_artifacts
        FIXTURES_SETUP ui_reference_system
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    add_test(NAME ui_parity_system COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_ui_parity.py compare
        --reference ${CMAKE_CURRENT_SOURCE_DIR}/docs/ui-reference/system-page.reference.json
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/docs/ui-reference/system-page.contract.json
        --reports ${CMAKE_BINARY_DIR}/ui-reports
        --output ${CMAKE_BINARY_DIR}/ui-reports/system-parity.json)
    set_tests_properties(ui_parity_system PROPERTIES FIXTURES_REQUIRED ui_reference_system)
    target_include_directories(menu_page_tests PRIVATE src tests)
    target_link_libraries(menu_page_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME menu_page COMMAND menu_page_tests -silent)
    set_tests_properties(menu_page PROPERTIES TIMEOUT 60
        RESOURCE_LOCK sevenzip_ui_artifacts ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/menu-page-settings.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(options_dialog_tests tests/options_dialog_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(options_dialog_tests PRIVATE src tests)
    target_link_libraries(options_dialog_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME options_dialog COMMAND options_dialog_tests -silent)
    qt_add_executable(options_integration_tests tests/options_integration_tests.cpp tests/layout_inspection.cpp)
    qt_add_executable(appearance_settings_tests tests/appearance_settings_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(appearance_settings_tests PRIVATE src tests)
    target_link_libraries(appearance_settings_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME appearance_settings COMMAND appearance_settings_tests -silent)
    set_tests_properties(appearance_settings PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    target_include_directories(options_integration_tests PRIVATE src)
    target_link_libraries(options_integration_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME options_integration COMMAND options_integration_tests -silent)
    qt_add_executable(options_keyboard_tests tests/options_keyboard_tests.cpp tests/layout_inspection.cpp)
    target_include_directories(options_keyboard_tests PRIVATE src)
    target_link_libraries(options_keyboard_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME options_keyboard COMMAND options_keyboard_tests -silent)
    set_tests_properties(options_keyboard PROPERTIES TIMEOUT 30
        RESOURCE_LOCK sevenzip_ui_artifacts ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(options_integration PROPERTIES TIMEOUT 30
        RESOURCE_LOCK sevenzip_ui_artifacts ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(options_dialog PROPERTIES TIMEOUT 60
        RESOURCE_LOCK sevenzip_ui_artifacts ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    get_target_property(options_window_sources settings_tests SOURCES)
    list(REMOVE_ITEM options_window_sources tests/settings_tests.cpp)
    get_target_property(options_window_libraries settings_tests LINK_LIBRARIES)
    if(SEVENZIP_CODEC_MODULE)
        qt_add_executable(toolbar_archive_workflow_tests tests/toolbar_archive_workflow_tests.cpp
            tests/layout_inspection.cpp ${options_window_sources})
        target_include_directories(toolbar_archive_workflow_tests PRIVATE src tests)
        target_link_libraries(toolbar_archive_workflow_tests PRIVATE ${options_window_libraries})
        target_compile_definitions(toolbar_archive_workflow_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
        add_test(NAME toolbar_archive_workflow COMMAND toolbar_archive_workflow_tests -silent)
        set_tests_properties(toolbar_archive_workflow PROPERTIES TIMEOUT 120
            RESOURCE_LOCK "sevenzip_native_ui;sevenzip_ui_artifacts" ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/toolbar-archive-workflow.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    endif()
    qt_add_executable(address_combo_tests tests/address_combo_tests.cpp
        tests/external_editor_fixture.cpp tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(address_combo_tests PRIVATE src tests)
    target_link_libraries(address_combo_tests PRIVATE ${options_window_libraries})
    add_test(NAME address_combo COMMAND address_combo_tests -silent)
    set_tests_properties(address_combo PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/address-combo.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_address_keys_tests tests/panel_address_keys_tests.cpp
        tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(panel_address_keys_tests PRIVATE src tests)
    target_link_libraries(panel_address_keys_tests PRIVATE ${options_window_libraries})
    add_test(NAME panel_address_keys COMMAND panel_address_keys_tests -silent)
    set_tests_properties(panel_address_keys PROPERTIES TIMEOUT 30 RESOURCE_LOCK sevenzip_ui_artifacts ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-address-keys.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(root_command_tests tests/root_command_tests.cpp
        tests/external_editor_fixture.cpp tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(root_command_tests PRIVATE src tests)
    target_link_libraries(root_command_tests PRIVATE ${options_window_libraries})
    add_test(NAME root_command COMMAND root_command_tests -silent)
    set_tests_properties(root_command PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/root-command.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(persisted_location_tests tests/persisted_location_tests.cpp
        tests/external_editor_fixture.cpp tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(persisted_location_tests PRIVATE src tests)
    target_link_libraries(persisted_location_tests PRIVATE ${options_window_libraries})
    add_test(NAME persisted_location COMMAND persisted_location_tests -silent)
    set_tests_properties(persisted_location PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/persisted-location.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    if(WIN32)
        qt_add_executable(native_address_navigation_tests tests/native_address_navigation_tests.cpp
            tests/fixtures/native_network_share.cpp tests/fixtures/widget_dialogs.cpp
            tests/layout_inspection.cpp ${options_window_sources})
        target_include_directories(native_address_navigation_tests PRIVATE src tests)
        target_link_libraries(native_address_navigation_tests PRIVATE ${options_window_libraries}
            netapi32 advapi32)
        add_test(NAME native_address_navigation COMMAND native_address_navigation_tests -silent)
        set_tests_properties(native_address_navigation PROPERTIES TIMEOUT 90 RUN_SERIAL TRUE
            RESOURCE_LOCK sevenzip_native_ui ENVIRONMENT
            "QT_QPA_PLATFORM=windows;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native-address-navigation.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
        qt_add_executable(native_persisted_locations_tests tests/native_persisted_locations_tests.cpp
            tests/fixtures/native_network_share.cpp tests/fixtures/native_network_share.h
            tests/fixtures/widget_dialogs.cpp tests/layout_inspection.cpp ${options_window_sources})
        target_include_directories(native_persisted_locations_tests PRIVATE src tests)
        target_link_libraries(native_persisted_locations_tests PRIVATE ${options_window_libraries}
            netapi32 advapi32)
        add_test(NAME native_persisted_locations COMMAND native_persisted_locations_tests -silent)
        set_tests_properties(native_persisted_locations PROPERTIES TIMEOUT 60 RUN_SERIAL TRUE ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native-persisted-locations.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
        qt_add_executable(native_network_navigation_tests tests/native_network_navigation_tests.cpp
            tests/fixtures/native_network_share.cpp tests/fixtures/native_network_share.h
            tests/layout_inspection.cpp ${options_window_sources})
        target_include_directories(native_network_navigation_tests PRIVATE src tests)
        target_link_libraries(native_network_navigation_tests PRIVATE ${options_window_libraries}
            netapi32 advapi32)
        add_test(NAME native_network_navigation COMMAND native_network_navigation_tests -silent)
        set_tests_properties(native_network_navigation PROPERTIES TIMEOUT 60 RUN_SERIAL TRUE ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native-network-navigation.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    endif()
    qt_add_executable(namespace_hash_tests tests/namespace_hash_tests.cpp
        tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(namespace_hash_tests PRIVATE src tests)
    target_link_libraries(namespace_hash_tests PRIVATE ${options_window_libraries})
    if(WIN32)
        target_sources(namespace_hash_tests PRIVATE tests/fixtures/read_only_virtual_disk.cpp
            tests/fixtures/read_only_virtual_disk.h)
        target_link_libraries(namespace_hash_tests PRIVATE virtdisk)
    endif()
    add_test(NAME namespace_hash COMMAND namespace_hash_tests -silent)
    set_tests_properties(namespace_hash PROPERTIES TIMEOUT 90 RUN_SERIAL TRUE ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/namespace-hash.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    if(WIN32)
        qt_add_executable(fs_drives_device_ui_tests tests/fs_drives_device_ui_tests.cpp
            tests/fixtures/read_only_virtual_disk.cpp tests/fixtures/read_only_virtual_disk.h
            tests/layout_inspection.cpp ${options_window_sources})
        target_include_directories(fs_drives_device_ui_tests PRIVATE src tests)
        target_link_libraries(fs_drives_device_ui_tests PRIVATE ${options_window_libraries} virtdisk)
        add_test(NAME fs_drives_device_ui COMMAND fs_drives_device_ui_tests -silent)
        set_tests_properties(fs_drives_device_ui PROPERTIES TIMEOUT 90 RUN_SERIAL TRUE ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/fs-drives-device-ui.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    endif()
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
    foreach(catalog_fixture_consumer language_tests language_ui_tests language_focus_tests)
        target_sources(${catalog_fixture_consumer} PRIVATE tests/fixtures/translation_catalog.cpp)
        target_compile_definitions(${catalog_fixture_consumer} PRIVATE TEST_LRELEASE_PATH="$<TARGET_FILE:Qt6::lrelease>")
    endforeach()
    add_test(NAME language_focus COMMAND language_focus_tests -silent)
    set_tests_properties(language_focus PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/language_focus.ini")
endif()

# Every current window consumer uses the same language presentation adapter.
if(BUILD_TESTING)
    get_target_property(codec_catalog_backend_sources archive_backend_tests SOURCES)
    list(REMOVE_ITEM codec_catalog_backend_sources tests/archive_backend_tests.cpp)
    get_target_property(codec_catalog_backend_libraries archive_backend_tests LINK_LIBRARIES)
    qt_add_executable(archive_workflow_tests tests/archive_workflow_tests.cpp
        ${codec_catalog_backend_sources}
        src/filetransferoperation.cpp src/filetransferoperation.h
        src/qtfiletransfertransaction.cpp src/qtfiletransfertransaction.h)
    target_include_directories(archive_workflow_tests PRIVATE src tests)
    target_link_libraries(archive_workflow_tests PRIVATE
        ${codec_catalog_backend_libraries} sevenzip_filesystem)
    add_test(NAME archive_workflow COMMAND archive_workflow_tests -silent)
    set_tests_properties(archive_workflow PROPERTIES TIMEOUT 60)
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
    qt_add_executable(agent_stream_update_tests tests/agent_stream_update_tests.cpp ${codec_update_sources})
    target_include_directories(agent_stream_update_tests PRIVATE src tests)
    target_link_libraries(agent_stream_update_tests PRIVATE sevenzip_location Qt6::Test)
    target_compile_definitions(agent_stream_update_tests PRIVATE
        "ALT_STREAMS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/streams/unicode-ads.wim\"")
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(agent_stream_update_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME agent_stream_update COMMAND agent_stream_update_tests -silent)
    set_tests_properties(agent_stream_update PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(native_update_path_tests tests/native_update_path_tests.cpp ${codec_update_sources})
    target_include_directories(native_update_path_tests PRIVATE src tests)
    target_link_libraries(native_update_path_tests PRIVATE sevenzip_location Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(native_update_path_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME native_update_path COMMAND native_update_path_tests -silent)
    set_tests_properties(native_update_path PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_add_executable(archive_transfer_backup_tests tests/archive_transfer_backup_tests.cpp
        ${codec_update_sources})
    target_include_directories(archive_transfer_backup_tests PRIVATE src tests)
    target_link_libraries(archive_transfer_backup_tests PRIVATE sevenzip_location Qt6::Test)
    add_test(NAME archive_transfer_backup COMMAND archive_transfer_backup_tests -silent)
    set_tests_properties(archive_transfer_backup PROPERTIES TIMEOUT 30)
    get_target_property(source_link_options_sources compression_preferences_tests SOURCES)
    list(REMOVE_ITEM source_link_options_sources tests/compression_preferences_tests.cpp)
    set(source_link_storage_sources ${codec_update_sources} ${source_link_options_sources})
    list(REMOVE_DUPLICATES source_link_storage_sources)
    qt_add_executable(source_link_storage_tests tests/source_link_storage_tests.cpp
        tests/layout_inspection.cpp ${source_link_storage_sources})
    target_include_directories(source_link_storage_tests PRIVATE src tests)
    target_link_libraries(source_link_storage_tests PRIVATE sevenzip_location Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(source_link_storage_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME source_link_storage COMMAND source_link_storage_tests -silent)
    set_tests_properties(source_link_storage PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(source_metadata_storage_tests tests/source_metadata_storage_tests.cpp
        tests/layout_inspection.cpp ${source_link_storage_sources})
    target_include_directories(source_metadata_storage_tests PRIVATE src tests)
    target_link_libraries(source_metadata_storage_tests PRIVATE sevenzip_location Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(source_metadata_storage_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME source_metadata_storage COMMAND source_metadata_storage_tests -silent)
    set_tests_properties(source_metadata_storage PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(codec_commands_ui_tests tests/codec_commands_ui_tests.cpp tests/layout_inspection.cpp ${options_window_sources})
    target_sources(codec_commands_ui_tests PRIVATE tests/fixtures/widget_dialogs.cpp)
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
    qt_add_executable(shell_menu_window_tests tests/shell_menu_window_tests.cpp tests/layout_inspection.cpp ${options_window_sources})
    if(WIN32)
        qt_add_executable(native_properties_window_tests tests/native_properties_window_tests.cpp
            ${options_window_sources})
        target_include_directories(native_properties_window_tests PRIVATE src tests)
        target_link_libraries(native_properties_window_tests PRIVATE ${options_window_libraries})
        add_test(NAME native_properties_window COMMAND native_properties_window_tests -silent)
        set_tests_properties(native_properties_window PROPERTIES TIMEOUT 240 ENVIRONMENT
            "QT_QPA_PLATFORM=windows;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native_properties_window.ini")
    endif()
    target_include_directories(shell_menu_window_tests PRIVATE src tests)
    target_link_libraries(shell_menu_window_tests PRIVATE ${options_window_libraries})
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(shell_menu_window_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME shell_menu_window COMMAND shell_menu_window_tests -silent)
    set_tests_properties(shell_menu_window PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/shell_menu_window.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(shell_update_window_tests tests/shell_update_window_tests.cpp
        tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(shell_update_window_tests PRIVATE src tests)
    target_link_libraries(shell_update_window_tests PRIVATE ${options_window_libraries})
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(shell_update_window_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME shell_update_window COMMAND shell_update_window_tests -silent)
    set_tests_properties(shell_update_window PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/shell_update_window.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(source_drop_add_tests tests/source_drop_add_tests.cpp ${options_window_sources})
    qt_add_executable(modal_refresh_window_tests tests/modal_refresh_window_tests.cpp ${options_window_sources})
    qt_add_executable(password_window_tests tests/password_window_tests.cpp tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(password_window_tests PRIVATE src tests)
    target_link_libraries(password_window_tests PRIVATE ${options_window_libraries})
    add_test(NAME password_window COMMAND password_window_tests -silent)
    set_tests_properties(password_window PROPERTIES TIMEOUT 30 RESOURCE_LOCK sevenzip_ui_artifacts ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/password_window.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    target_include_directories(modal_refresh_window_tests PRIVATE src tests)
    target_link_libraries(modal_refresh_window_tests PRIVATE ${options_window_libraries})
    add_test(NAME modal_refresh_window COMMAND modal_refresh_window_tests -silent)
    set_tests_properties(modal_refresh_window PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/modal_refresh_window.ini")
    target_include_directories(source_drop_add_tests PRIVATE src tests)
    target_link_libraries(source_drop_add_tests PRIVATE ${options_window_libraries})
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(source_drop_add_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME source_drop_add COMMAND source_drop_add_tests -silent)
    set_tests_properties(source_drop_add PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/source_drop_add.ini")
    qt_add_executable(benchmark_command_tests tests/benchmark_command_tests.cpp ${options_window_sources})
    qt_add_executable(link_command_tests tests/link_command_tests.cpp ${options_window_sources})
    target_include_directories(link_command_tests PRIVATE src tests)
    target_link_libraries(link_command_tests PRIVATE ${options_window_libraries})
    add_test(NAME link_command COMMAND link_command_tests -silent)
    qt_add_executable(alt_streams_command_tests tests/alt_streams_command_tests.cpp tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(alt_streams_command_tests PRIVATE src tests)
    target_link_libraries(alt_streams_command_tests PRIVATE ${options_window_libraries})
    add_test(NAME alt_streams_command COMMAND alt_streams_command_tests -silent)
    qt_add_executable(fs_provider_command_tests tests/fs_provider_command_tests.cpp tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(fs_provider_command_tests PRIVATE src tests)
    target_link_libraries(fs_provider_command_tests PRIVATE ${options_window_libraries})
    add_test(NAME fs_provider_command COMMAND fs_provider_command_tests -silent)
    qt_add_executable(view_menu_tests tests/view_menu_tests.cpp tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(view_menu_tests PRIVATE src tests)
    target_link_libraries(view_menu_tests PRIVATE ${options_window_libraries})
    add_test(NAME view_menu COMMAND view_menu_tests -silent)
    set_tests_properties(view_menu PROPERTIES TIMEOUT 30 RESOURCE_LOCK sevenzip_ui_artifacts ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/view-menu.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(native_archive_ads_copy_tests tests/native_archive_ads_copy_tests.cpp tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(native_archive_ads_copy_tests PRIVATE src tests)
    target_link_libraries(native_archive_ads_copy_tests PRIVATE ${options_window_libraries})
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(native_archive_ads_copy_tests PRIVATE "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME native_archive_ads_copy COMMAND native_archive_ads_copy_tests -silent)
    set_tests_properties(native_archive_ads_copy PROPERTIES TIMEOUT 60 RESOURCE_LOCK sevenzip_ui_artifacts ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native-archive-ads-copy.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(fs_provider_command PROPERTIES TIMEOUT 60 RESOURCE_LOCK sevenzip_ui_artifacts ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/fs-provider-command.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(alt_streams_archive_ui_tests tests/alt_streams_archive_ui_tests.cpp tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(alt_streams_archive_ui_tests PRIVATE src tests)
    target_link_libraries(alt_streams_archive_ui_tests PRIVATE ${options_window_libraries})
    target_compile_definitions(alt_streams_archive_ui_tests PRIVATE
        "ALT_STREAMS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/streams/unicode-ads.wim\"")
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(alt_streams_archive_ui_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME alt_streams_archive_ui COMMAND alt_streams_archive_ui_tests -silent)
    qt_add_executable(agent_stream_copy_ui_tests tests/agent_stream_copy_ui_tests.cpp
        tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(agent_stream_copy_ui_tests PRIVATE src tests)
    target_link_libraries(agent_stream_copy_ui_tests PRIVATE ${options_window_libraries})
    target_compile_definitions(agent_stream_copy_ui_tests PRIVATE
        "ALT_STREAMS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/streams/unicode-ads.wim\"")
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(agent_stream_copy_ui_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME agent_stream_copy_ui COMMAND agent_stream_copy_ui_tests -silent)
    set_tests_properties(agent_stream_copy_ui PROPERTIES TIMEOUT 60
        RESOURCE_LOCK sevenzip_ui_artifacts ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/agent-stream-copy-ui.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    set_tests_properties(alt_streams_archive_ui PROPERTIES TIMEOUT 45 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/alt-streams-archive-ui.ini")
    set_tests_properties(alt_streams_command PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/alt-streams-command.ini")
    if(WIN32 AND SEVENZIP_CODEC_MODULE)
        qt_add_executable(native_zone_copy_tests tests/native_zone_copy_tests.cpp
            tests/layout_inspection.cpp ${options_window_sources})
        target_include_directories(native_zone_copy_tests PRIVATE src tests)
        target_link_libraries(native_zone_copy_tests PRIVATE ${options_window_libraries})
        target_compile_definitions(native_zone_copy_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
        add_test(NAME native_zone_copy COMMAND native_zone_copy_tests -silent)
        set_tests_properties(native_zone_copy PROPERTIES TIMEOUT 60
            RESOURCE_LOCK "sevenzip_native_ui;sevenzip_ui_artifacts" ENVIRONMENT
            "QT_QPA_PLATFORM=windows;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native-zone-copy.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    endif()
    set_tests_properties(link_command PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/link_command.ini")
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
    qt_add_executable(filesystem_comment_tests tests/filesystem_comment_tests.cpp tests/layout_inspection.cpp ${options_window_sources})
    qt_add_executable(source_context_menu_tests tests/source_context_menu_tests.cpp tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(source_context_menu_tests PRIVATE src tests)
    target_link_libraries(source_context_menu_tests PRIVATE ${options_window_libraries})
    add_test(NAME source_context_menu COMMAND source_context_menu_tests -silent)
    set_tests_properties(source_context_menu PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/source_context_menu.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(source_selection_tests tests/source_selection_tests.cpp tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(source_selection_tests PRIVATE src tests)
    target_link_libraries(source_selection_tests PRIVATE ${options_window_libraries})
    add_test(NAME source_selection COMMAND source_selection_tests -silent)
    set_tests_properties(source_selection PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/source_selection.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(source_delete_tests tests/source_delete_tests.cpp tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(source_delete_tests PRIVATE src tests)
    target_link_libraries(source_delete_tests PRIVATE ${options_window_libraries})
    add_test(NAME source_delete COMMAND source_delete_tests -silent)
    set_tests_properties(source_delete PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/source_delete.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(source_create_tests tests/source_create_tests.cpp tests/layout_inspection.cpp ${options_window_sources})
    target_include_directories(source_create_tests PRIVATE src tests)
    target_link_libraries(source_create_tests PRIVATE ${options_window_libraries})
    add_test(NAME source_create COMMAND source_create_tests -silent)
    set_tests_properties(source_create PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/source_create.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    target_include_directories(filesystem_comment_tests PRIVATE src tests)
    target_link_libraries(filesystem_comment_tests PRIVATE ${options_window_libraries})
    add_test(NAME filesystem_comment COMMAND filesystem_comment_tests -silent)
    set_tests_properties(filesystem_comment PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/filesystem_comment.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
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
    target_compile_definitions(native_zip_writer_tests PRIVATE
        "SEVENZIP_ZIP_VERIFY_ENGINE=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/7zz${CMAKE_EXECUTABLE_SUFFIX}\"")
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
    qt_add_executable(native_archive_properties_tests tests/native_archive_properties_tests.cpp
        tests/layout_inspection.cpp ${codec_update_sources})
    target_include_directories(native_archive_properties_tests PRIVATE src tests)
    target_link_libraries(native_archive_properties_tests PRIVATE sevenzip_location Qt6::Test)
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(native_archive_properties_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME native_archive_properties COMMAND native_archive_properties_tests -silent)
    set_tests_properties(native_archive_properties PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(device_input_tests tests/device_input_tests.cpp)
    target_include_directories(device_input_tests PRIVATE src)
    target_link_libraries(device_input_tests PRIVATE sevenzip_filesystem Qt6::Test)
    add_test(NAME device_input COMMAND device_input_tests -silent)
    set_tests_properties(device_input PROPERTIES TIMEOUT 15)
    if(WIN32)
        qt_add_executable(device_access_win_tests tests/device_access_win_tests.cpp)
        target_link_libraries(device_access_win_tests PRIVATE sevenzip_device_access Qt6::Test)
        add_test(NAME device_access_win COMMAND device_access_win_tests -silent)
        set_tests_properties(device_access_win PROPERTIES TIMEOUT 30)
        qt_add_executable(device_pipe_win_tests tests/device_pipe_win_tests.cpp)
        target_link_libraries(device_pipe_win_tests PRIVATE sevenzip_device_access Qt6::Test)
        add_test(NAME device_pipe_win COMMAND device_pipe_win_tests -silent)
        set_tests_properties(device_pipe_win PROPERTIES TIMEOUT 20)
    endif()
    if(APPLE OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
        qt_add_executable(device_access_tests tests/device_access_tests.cpp)
        target_link_libraries(device_access_tests PRIVATE sevenzip_device_access Qt6::Test)
        add_test(NAME device_access COMMAND device_access_tests -silent)
        set_tests_properties(device_access PROPERTIES TIMEOUT 15)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        qt_add_executable(udisks_device_tests tests/udisks_device_tests.cpp)
        target_include_directories(udisks_device_tests PRIVATE tests)
        target_link_libraries(udisks_device_tests PRIVATE sevenzip_device_access private_bus_fixture Qt6::Test)
        add_test(NAME udisks_device COMMAND udisks_device_tests -silent)
        set_tests_properties(udisks_device PROPERTIES TIMEOUT 30)
    endif()
    qt_add_executable(archive_device_tests tests/archive_device_tests.cpp)
    target_include_directories(archive_device_tests PRIVATE src)
    target_link_libraries(archive_device_tests PRIVATE sevenzip_filesystem Qt6::Test)
    target_compile_definitions(archive_device_tests PRIVATE
        "DEVICE_FIXTURE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/devices\"")
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(archive_device_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME archive_device COMMAND archive_device_tests -silent)
    add_dependencies(archive_device_tests sevenzip_mbr_runtime)
    set_tests_properties(archive_device PROPERTIES TIMEOUT 30)
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
    target_include_directories(codec_probe_fixture PRIVATE third_party/7zip/CPP)
    target_link_libraries(codec_probe_fixture PRIVATE Qt6::Core)
    target_compile_definitions(codec_catalog_tests PRIVATE
        "CODEC_PROBE_FIXTURE=\"$<TARGET_FILE:codec_probe_fixture>\"")
    add_dependencies(codec_catalog_tests codec_probe_fixture)
    add_library(slow_codec_fixture MODULE tests/fixtures/slow_codec_fixture.cpp)
    target_include_directories(slow_codec_fixture PRIVATE third_party/7zip/CPP)
    target_link_libraries(slow_codec_fixture PRIVATE Qt6::Core)
    set_target_properties(slow_codec_fixture PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX "")
    if(WIN32)
        target_sources(slow_codec_fixture PRIVATE tests/incompatible_codec_fixture.def)
    endif()
    qt_add_executable(codec_probe_tests tests/codec_probe_tests.cpp)
    if(WIN32)
        target_sources(codec_probe_tests PRIVATE tests/fixtures/delayed_process_start.cpp
            tests/fixtures/delayed_process_start.h)
    endif()
    target_include_directories(codec_probe_tests PRIVATE src)
    target_link_libraries(codec_probe_tests PRIVATE sevenzip_filesystem Qt6::Test)
    target_compile_definitions(codec_probe_tests PRIVATE
        "CODEC_PROBE_FIXTURE=\"$<TARGET_FILE:codec_probe_fixture>\""
        "SLOW_CODEC_FIXTURE=\"$<TARGET_FILE:slow_codec_fixture>\"")
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(codec_probe_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_dependencies(codec_probe_tests codec_probe_fixture slow_codec_fixture)
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
        target_sources(${language_consumer} PRIVATE src/mainwindow_language.cpp src/qtpanelcontextmenu.cpp)
    endif()
endforeach()

if(BUILD_TESTING)
    get_target_property(startup_path_sources ui_navigation_tests SOURCES)
    list(REMOVE_ITEM startup_path_sources tests/ui_navigation_tests.cpp)
    get_target_property(startup_path_libraries ui_navigation_tests LINK_LIBRARIES)
    qt_add_executable(startup_path_tests tests/startup_path_tests.cpp ${startup_path_sources})
    target_include_directories(startup_path_tests PRIVATE src tests)
    target_link_libraries(startup_path_tests PRIVATE ${startup_path_libraries})
    add_test(NAME startup_path COMMAND startup_path_tests -silent)
    set_tests_properties(startup_path PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/startup-path-settings.ini"
        TIMEOUT 30)
endif()

if(BUILD_TESTING)
    get_target_property(metadata_ui_sources ui_update_modes_tests SOURCES)
    list(REMOVE_ITEM metadata_ui_sources tests/ui_update_modes_tests.cpp)
    get_target_property(metadata_ui_libraries ui_update_modes_tests LINK_LIBRARIES)
    qt_add_executable(filesystem_read_ui_tests tests/filesystem_read_ui_tests.cpp
        tests/exfat_fixture.cpp tests/layout_inspection.cpp ${metadata_ui_sources})
    target_include_directories(filesystem_read_ui_tests PRIVATE src tests)
    target_link_libraries(filesystem_read_ui_tests PRIVATE ${metadata_ui_libraries})
    target_compile_definitions(filesystem_read_ui_tests PRIVATE
        "SEVENZIP_SQUASHFS_MODULE=\"$<TARGET_FILE:sevenzip_squashfs>\""
        "SEVENZIP_SQUASHFS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/squashfs_fixture.py\""
        "SEVENZIP_EXFAT_MODULE=\"$<TARGET_FILE:sevenzip_exfat>\""
        "SEVENZIP_FAT_MODULE=\"$<TARGET_FILE:sevenzip_fat>\""
        "SEVENZIP_FAT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fat_fixture.py\""
        "SEVENZIP_EXFAT_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_HFS_MODULE=\"$<TARGET_FILE:sevenzip_hfs>\""
        "SEVENZIP_NTFS_MODULE=\"$<TARGET_FILE:sevenzip_ntfs>\""
        "SEVENZIP_NTFS_CORE=\"$<TARGET_FILE:sevenzip_ntfs>\""
        "SEVENZIP_NTFS_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_NTFS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/ntfs_fixture.py\""
        "SEVENZIP_EXT_MODULE=\"$<TARGET_FILE:sevenzip_ext>\""
        "SEVENZIP_EXT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/ext_fixture.py\""
        "SEVENZIP_HFS_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_HFS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/hfs_fixture.py\"")
    add_dependencies(filesystem_read_ui_tests sevenzip_exfat sevenzip_fat_runtime sevenzip_hfs_runtime sevenzip_ntfs_runtime sevenzip_ext_runtime sevenzip_squashfs_runtime)
    add_test(NAME filesystem_squashfs_read_ui COMMAND filesystem_read_ui_tests -silent confirmsPartialSquashfsAttributes)
    add_test(NAME filesystem_squashfs_directory_ui COMMAND filesystem_read_ui_tests -silent confirmsPartialSquashfsDirectory)
    add_test(NAME filesystem_squashfs_inode_ui COMMAND filesystem_read_ui_tests -silent confirmsPartialSquashfsInodes)
    add_test(NAME filesystem_squashfs_index_ui COMMAND filesystem_read_ui_tests -silent confirmsPartialSquashfsIndexes)
    add_test(NAME filesystem_read_ui COMMAND filesystem_read_ui_tests -silent deletedRecoveryConsent confirmsAndCopiesFlaggedVolume)
    add_test(NAME filesystem_read_ui_ru COMMAND filesystem_read_ui_tests -silent confirmsAndCopiesFlaggedVolumeRu)
    add_test(NAME filesystem_security_read_ui COMMAND filesystem_read_ui_tests -silent confirmsPartialSecurityStream)
    add_test(NAME filesystem_fat_read_ui COMMAND filesystem_read_ui_tests -silent confirmsLimitedFat)
    add_test(NAME filesystem_fat_directory_ui COMMAND filesystem_read_ui_tests -silent confirmsPartialFatDirectory)
    add_test(NAME filesystem_fat_file_ui COMMAND filesystem_read_ui_tests -silent confirmsPartialFatFile)
    set_tests_properties(filesystem_squashfs_index_ui filesystem_squashfs_inode_ui filesystem_squashfs_directory_ui filesystem_squashfs_read_ui filesystem_read_ui filesystem_read_ui_ru filesystem_security_read_ui filesystem_fat_read_ui filesystem_fat_directory_ui filesystem_fat_file_ui PROPERTIES TIMEOUT 60 RESOURCE_LOCK sevenzip_ui_artifacts
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    if(APPLE)
        set_property(TEST filesystem_squashfs_index_ui filesystem_squashfs_inode_ui filesystem_squashfs_directory_ui filesystem_squashfs_read_ui filesystem_read_ui filesystem_read_ui_ru filesystem_security_read_ui filesystem_fat_read_ui filesystem_fat_directory_ui filesystem_fat_file_ui PROPERTY ENVIRONMENT
            "QT_QPA_PLATFORM=cocoa;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    endif()
    qt_add_executable(associated_window_tests tests/associated_window_tests.cpp
        tests/fixtures/widget_dialogs.cpp
        src/qtfilemanagermain.cpp ${metadata_ui_sources})
    target_include_directories(associated_window_tests PRIVATE src tests)
    target_link_libraries(associated_window_tests PRIVATE ${metadata_ui_libraries})
    target_compile_definitions(associated_window_tests PRIVATE
        "CODEC_PROBE_FIXTURE=\"$<TARGET_FILE:codec_probe_fixture>\"")
    add_dependencies(associated_window_tests codec_probe_fixture)
    add_test(NAME associated_window COMMAND associated_window_tests -silent)
    set_tests_properties(associated_window PROPERTIES TIMEOUT 30 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/associated-window.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    if(APPLE)
        add_library(mac_document_main OBJECT src/main.cpp)
        target_compile_definitions(mac_document_main PRIVATE main=SevenzipDesktopMain)
        target_include_directories(mac_document_main PRIVATE src)
        target_link_libraries(mac_document_main PRIVATE ${metadata_ui_libraries})
        qt_add_executable(mac_document_entry_probe tests/fixtures/mac_document_entry_probe.cpp
            src/qtfilemanagermain.cpp src/qtguimain.cpp ${metadata_ui_sources}
            $<TARGET_OBJECTS:mac_document_main>)
        target_include_directories(mac_document_entry_probe PRIVATE src tests)
        target_link_libraries(mac_document_entry_probe PRIVATE ${metadata_ui_libraries})
        add_executable(mac_document_launcher tests/fixtures/mac_document_launcher.mm)
        target_link_libraries(mac_document_launcher PRIVATE "-framework AppKit")
        add_test(NAME macos_document_startup COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/mac_document_startup_test.py"
            --probe "$<TARGET_FILE:mac_document_entry_probe>"
            --launcher "$<TARGET_FILE:mac_document_launcher>")
        set_tests_properties(macos_document_startup PROPERTIES TIMEOUT 90
            RESOURCE_LOCK sevenzip_native_ui)
    endif()
    qt_add_executable(archive_metadata_ui_tests tests/archive_metadata_ui_tests.cpp ${metadata_ui_sources})
    qt_add_executable(device_ui_tests tests/device_ui_tests.cpp tests/layout_inspection.cpp ${metadata_ui_sources})
    target_include_directories(device_ui_tests PRIVATE src tests)
    target_link_libraries(device_ui_tests PRIVATE ${metadata_ui_libraries})
    target_compile_definitions(device_ui_tests PRIVATE
        "DEVICE_FIXTURE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/devices\"")
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(device_ui_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME device_ui COMMAND device_ui_tests -silent)
    foreach(ext_consumer archive_device_tests device_folder_tests device_ui_tests)
        add_dependencies(${ext_consumer} sevenzip_ext_runtime)
    endforeach()
    set_tests_properties(device_ui PROPERTIES TIMEOUT 90 RESOURCE_LOCK sevenzip_ui_artifacts
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/device-ui.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    if(APPLE)
        set_property(TEST device_ui PROPERTY ENVIRONMENT
            "QT_QPA_PLATFORM=cocoa;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/device-ui.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    endif()
    target_include_directories(archive_metadata_ui_tests PRIVATE src tests)
    target_link_libraries(archive_metadata_ui_tests PRIVATE ${metadata_ui_libraries})
    if(SEVENZIP_CODEC_MODULE)
        if(WIN32)
            add_executable(native_association_process tests/fixtures/native_association_process.cpp)
            qt_add_executable(native_association_tests tests/native_association_tests.cpp
                ${metadata_ui_sources})
            target_include_directories(native_association_tests PRIVATE src tests)
            target_link_libraries(native_association_tests PRIVATE ${metadata_ui_libraries})
            target_compile_definitions(native_association_tests PRIVATE
                "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\""
                "SEVENZIP_ASSOC_HELPER=\"$<TARGET_FILE:native_association_process>\"")
            add_dependencies(native_association_tests native_association_process)
            foreach(association_case IN ITEMS handle disk browser workflow)
                if(association_case STREQUAL "handle")
                    set(association_function nativeShellReturnsWaitableProcess)
                elseif(association_case STREQUAL "disk")
                    set(association_function diskOpenKeepsOriginalAndDoesNotWait)
                elseif(association_case STREQUAL "browser")
                    set(association_function temporaryBrowserClosesHandleAndRetainsProtection)
                else()
                    set(association_function archiveOpenCleansCopyAfterNativeCompletion)
                endif()
                add_test(NAME native_association_${association_case}
                    COMMAND native_association_tests ${association_function} -silent)
                set_tests_properties(native_association_${association_case} PROPERTIES TIMEOUT 90
                    RESOURCE_LOCK sevenzip_native_ui
                    ENVIRONMENT "QT_QPA_PLATFORM=windows;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native-association-${association_case}.ini")
            endforeach()
        endif()
        qt_add_executable(file_manager_entry_probe tests/fixtures/file_manager_entry_probe.cpp
            src/qtfilemanagermain.cpp ${metadata_ui_sources})
        target_include_directories(file_manager_entry_probe PRIVATE src tests)
        target_link_libraries(file_manager_entry_probe PRIVATE ${metadata_ui_libraries})
        add_library(file_manager_ram_source_fixture OBJECT
            src/CPP/7zip/UI/FileManager/FM.cpp src/CPP/7zip/UI/FileManager/PanelItemOpen.cpp)
        target_include_directories(file_manager_ram_source_fixture PRIVATE src tests)
        target_link_libraries(file_manager_ram_source_fixture PRIVATE ${metadata_ui_libraries})
        target_compile_definitions(file_manager_ram_source_fixture PRIVATE
            GetRamSize=FixtureGetRamSize)
        qt_add_executable(file_manager_ram_probe tests/fixtures/file_manager_ram_probe.cpp
            $<TARGET_OBJECTS:file_manager_ram_source_fixture>
            src/qtfilemanagermain.cpp ${metadata_ui_sources})
        target_include_directories(file_manager_ram_probe PRIVATE src tests)
        target_link_libraries(file_manager_ram_probe PRIVATE ${metadata_ui_libraries})
        qt_add_executable(file_manager_ram_tests tests/file_manager_ram_tests.cpp)
        target_include_directories(file_manager_ram_tests PRIVATE src tests)
        target_link_libraries(file_manager_ram_tests PRIVATE ${metadata_ui_libraries})
        target_compile_definitions(file_manager_ram_tests PRIVATE
            "SEVENZIP_RAM_PROBE=\"$<TARGET_FILE:file_manager_ram_probe>\"")
        add_dependencies(file_manager_ram_tests file_manager_ram_probe)
        add_test(NAME file_manager_ram COMMAND file_manager_ram_tests -silent)
        set_tests_properties(file_manager_ram PROPERTIES TIMEOUT 120 ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/file-manager-ram.ini")
        if(NOT WIN32)
            qt_add_executable(unix_gui_entry_tests tests/unix_gui_entry_tests.cpp)
            target_include_directories(unix_gui_entry_tests PRIVATE src tests)
            target_link_libraries(unix_gui_entry_tests PRIVATE sevenzip_common Qt6::Test)
            target_compile_definitions(unix_gui_entry_tests PRIVATE
                "SEVENZIP_NATIVE_GUI=\"$<TARGET_FILE_DIR:sevenzip_desktop>/7zG\"")
            add_dependencies(unix_gui_entry_tests sevenzip_desktop)
            add_test(NAME unix_gui_entry COMMAND unix_gui_entry_tests -silent)
            set_tests_properties(unix_gui_entry PROPERTIES TIMEOUT 180 ENVIRONMENT
                "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/unix-gui-entry.ini")
        endif()
        qt_add_executable(file_manager_startup_tests tests/file_manager_startup_tests.cpp ${metadata_ui_sources})
        target_include_directories(file_manager_startup_tests PRIVATE src tests)
        target_link_libraries(file_manager_startup_tests PRIVATE ${metadata_ui_libraries})
        target_compile_definitions(file_manager_startup_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\""
            "SEVENZIP_FM_PROBE=\"$<TARGET_FILE:file_manager_entry_probe>\"")
        add_dependencies(file_manager_startup_tests file_manager_entry_probe)
        if(WIN32)
            target_compile_definitions(file_manager_startup_tests PRIVATE
                "SEVENZIP_NATIVE_FM=\"$<TARGET_FILE_DIR:sevenzip_desktop>/7zFM.exe\"")
            add_dependencies(file_manager_startup_tests sevenzip_desktop)
        endif()
        add_test(NAME file_manager_startup COMMAND file_manager_startup_tests -silent)
        set_tests_properties(file_manager_startup PROPERTIES TIMEOUT 90 ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/file-manager-startup.ini")
    endif()
    if(WIN32 AND SEVENZIP_CODEC_MODULE)
        qt_add_executable(native_shell_email_tests tests/native_shell_email_tests.cpp
            tests/fixtures/native_mapi_loader.cpp tests/layout_inspection.cpp ${metadata_ui_sources})
        target_include_directories(native_shell_email_tests PRIVATE src tests)
        target_link_libraries(native_shell_email_tests PRIVATE ${metadata_ui_libraries})
        target_compile_definitions(native_shell_email_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\""
            "NATIVE_MAPI_WIDE=\"$<TARGET_FILE:native_mapi_wide>\""
            "NATIVE_MAPI_MISSING=\"$<TARGET_FILE:native_mapi_missing>\"")
        add_dependencies(native_shell_email_tests native_mapi_wide native_mapi_missing)
        add_test(NAME native_shell_email COMMAND native_shell_email_tests -silent)
        set_tests_properties(native_shell_email PROPERTIES TIMEOUT 180 RESOURCE_LOCK sevenzip_ui_artifacts
            ENVIRONMENT "SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/native-shell-email.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    endif()
    if(SEVENZIP_CODEC_MODULE)
        target_compile_definitions(archive_metadata_ui_tests PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endif()
    add_test(NAME archive_metadata_ui COMMAND archive_metadata_ui_tests -silent)
    set_tests_properties(archive_metadata_ui PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/archive-metadata-ui.ini")
endif()

if(BUILD_TESTING)
    get_target_property(large_list_window_sources flat_view_tests SOURCES)
    list(REMOVE_ITEM large_list_window_sources tests/flat_view_tests.cpp)
    get_target_property(large_list_window_libraries flat_view_tests LINK_LIBRARIES)
    qt_add_executable(large_list_window_tests tests/large_list_window_tests.cpp ${large_list_window_sources})
    target_include_directories(large_list_window_tests PRIVATE src tests)
    target_link_libraries(large_list_window_tests PRIVATE ${large_list_window_libraries})
    add_test(NAME large_list_window COMMAND large_list_window_tests -silent)
    # Four measured projections and their refreshes each have a 15s deadline;
    # leave room for fixture I/O and the separate lifecycle/selection cases.
    # Timing the real GUI heartbeat must not compete with other CTest processes.
    set_tests_properties(large_list_window PROPERTIES TIMEOUT 240 RUN_SERIAL TRUE ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/large-list-window.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    get_target_property(worker_window_progress_sources ui_copy_same_tests SOURCES)
    list(REMOVE_ITEM worker_window_progress_sources tests/ui_copy_same_tests.cpp)
    get_target_property(worker_window_progress_libraries ui_copy_same_tests LINK_LIBRARIES)
    qt_add_executable(worker_window_progress_tests tests/worker_window_progress_tests.cpp ${worker_window_progress_sources})
    target_include_directories(worker_window_progress_tests PRIVATE src tests)
    target_link_libraries(worker_window_progress_tests PRIVATE ${worker_window_progress_libraries})
    add_test(NAME worker_window_progress COMMAND worker_window_progress_tests -silent)
    set_tests_properties(worker_window_progress PROPERTIES TIMEOUT 120 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/worker-window-progress.ini")
endif()

if(BUILD_TESTING)
    get_target_property(panel_folder_sync_sources panel_key_tests SOURCES)
    list(REMOVE_ITEM panel_folder_sync_sources tests/panel_key_tests.cpp)
    get_target_property(panel_folder_sync_libraries panel_key_tests LINK_LIBRARIES)
    qt_add_executable(panel_shortcut_tests tests/panel_shortcut_tests.cpp
        ${panel_folder_sync_sources})
    target_include_directories(panel_shortcut_tests PRIVATE src tests)
    target_link_libraries(panel_shortcut_tests PRIVATE ${panel_folder_sync_libraries})
    add_test(NAME panel_shortcut COMMAND panel_shortcut_tests -silent)
    set_tests_properties(panel_shortcut PROPERTIES TIMEOUT 60 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-shortcut.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_folder_sync_tests tests/panel_folder_sync_tests.cpp
        ${panel_folder_sync_sources})
    target_include_directories(panel_folder_sync_tests PRIVATE src tests)
    target_link_libraries(panel_folder_sync_tests PRIVATE ${panel_folder_sync_libraries})
    add_test(NAME panel_folder_sync COMMAND panel_folder_sync_tests -silent)
    set_tests_properties(panel_folder_sync PROPERTIES TIMEOUT 90 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-folder-sync.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_key_source_tests tests/panel_key_source_tests.cpp
        ${panel_folder_sync_sources})
    target_include_directories(panel_key_source_tests PRIVATE src tests)
    target_link_libraries(panel_key_source_tests PRIVATE ${panel_folder_sync_libraries})
    add_test(NAME panel_key_source COMMAND panel_key_source_tests -silent)
    set_tests_properties(panel_key_source PROPERTIES TIMEOUT 90 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-key-source.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    qt_add_executable(panel_list_key_tests tests/panel_list_key_tests.cpp
        ${panel_folder_sync_sources})
    target_include_directories(panel_list_key_tests PRIVATE src tests)
    target_link_libraries(panel_list_key_tests PRIVATE ${panel_folder_sync_libraries})
    target_compile_definitions(panel_list_key_tests PRIVATE
        SEVENZIP_CODEC_MODULE="${SEVENZIP_CODEC_MODULE}")
    add_test(NAME panel_list_key COMMAND panel_list_key_tests -silent)
    set_tests_properties(panel_list_key PROPERTIES TIMEOUT 90 ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/panel-list-key.ini")
endif()

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
    set(sevenzip_test_target ${sevenzip_test}_tests)
    if(sevenzip_test STREQUAL "bookmarks")
        set(sevenzip_test_target bookmark_tests)
    elseif(sevenzip_test MATCHES "^native_rename_(2|301)$")
        set(sevenzip_test_target native_rename_publication_tests)
    elseif(sevenzip_test MATCHES "^native_external_(2|301)$")
        set(sevenzip_test_target native_external_publication_tests)
    elseif(sevenzip_test MATCHES "^native_nested_(stream|memory|disk)$")
        set(sevenzip_test_target native_nested_publication_tests)
    elseif(sevenzip_test MATCHES "^native_association_(handle|disk|browser|workflow)$")
        set(sevenzip_test_target native_association_tests)
    endif()
    if(TARGET ${sevenzip_test_target})
        get_target_property(sevenzip_test_sources ${sevenzip_test_target} SOURCES)
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

# Add now reaches the source modal through CPanel and requires its native
# codec metadata even in selection/cancellation-only window tests.
if(BUILD_TESTING AND SEVENZIP_CODEC_MODULE)
    foreach(sevenzip_source_add_test IN ITEMS
            filesystem_selection_runtime_tests ui_operations_tests ui_update_modes_tests
            drag_drop_add_ui_tests)
        target_compile_definitions(${sevenzip_source_add_test} PRIVATE
            "SEVENZIP_CODEC_MODULE=\"${SEVENZIP_CODEC_MODULE}\"")
    endforeach()
endif()

# Startup isolation and test fixtures inspect the original consumer source lists.
# Share production objects only after those registrations are complete.
include(cmake/SharedPortObjects.cmake)
include(cmake/CompactTestBinaries.cmake)
include(cmake/SlowTests.cmake)
if(BUILD_TESTING)
    add_test(NAME ctest_selection COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/ctest_selection_tests.py")
endif()
if(BUILD_TESTING AND (MSVC OR CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    add_test(NAME compact_test_binaries COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/compact_test_binaries_tests.py")
endif()

if(BUILD_TESTING)
    qt_add_executable(widget_dialog_fixture_tests tests/widget_dialog_fixture_tests.cpp
        tests/fixtures/widget_dialogs.cpp)
    target_include_directories(widget_dialog_fixture_tests PRIVATE tests)
    target_link_libraries(widget_dialog_fixture_tests PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME widget_dialog_fixture COMMAND widget_dialog_fixture_tests -silent)
    set_tests_properties(widget_dialog_fixture PROPERTIES TIMEOUT 10
        RESOURCE_LOCK sevenzip_native_ui ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    foreach(sevenzip_widget_dialog_target IN ITEMS
            browse_dialog_tests nested_update_tests archive_progress_tests progress_cancel_tests
            nested_edit_chain_tests archive_external_edit_tests archive_external_security_tests
            archive_external_lifetime_tests checksum_tests source_link_storage_tests
            options_dialog_tests system_page_tests flat_view_tests filesystem_reload_tests
            toolbar_archive_workflow_tests shell_update_window_tests shell_menu_window_tests source_drop_add_tests
            native_hash_ui_tests native_rar_ui_tests panel_list_key_tests source_context_menu_tests
            panel_copy_reload_tests native_drop_publication_tests panel_shortcut_tests source_delete_tests
            copy_publication_order_tests native_parent_publication_tests panel_key_source_tests)
        target_sources(${sevenzip_widget_dialog_target} PRIVATE tests/fixtures/widget_dialogs.cpp)
    endforeach()
endif()

# The Windows offscreen plugin uses FreeType rather than the native Windows
# font database. Without a separately supplied font directory its fallback box
# metrics do not describe the application's controls. Compare these dialog and
# consumer tests using the same platform/fonts/style as the Windows application.
if(BUILD_TESTING AND WIN32)
    if(TEST toolbar_archive_workflow)
        set_property(TEST toolbar_archive_workflow PROPERTY ENVIRONMENT
            "QT_QPA_PLATFORM=windows;SEVENZIP_SETTINGS_PATH=${CMAKE_BINARY_DIR}/toolbar-archive-workflow.ini;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    endif()
    foreach(sevenzip_native_ui_test IN ITEMS
            native_parent_publication native_drop_publication native_rename_2 native_rename_301 native_external_2 native_external_301 native_nested_stream native_nested_memory native_nested_disk
            native_renderer_failure native_renderer_copy interarchive_refresh panel_creation_publication
            source_compress_dialog source_extract_advanced source_browse_dialog shell_archive_services shell_menu_window shell_update_window
            compression compression_advanced compression_path dialog_path_rules compression_error_policy compression_profile_selection compression_method_state compression_preferences compression_resources language_ui
            codec_commands_ui codec_creation_ui ui_geometry dialog_size
            options_dialog options_integration options_keyboard options_window appearance_settings system_page menu_page help_source help_ui temp_cleanup context_dialog
            filesystem_selection_runtime ui_operations ui_copy_same ui_copy_routing app_copy_routing ui_update_modes archive_metadata_ui source_drop_add
            drag_drop_add_ui modal_refresh_window
            main_shell panel_shell panel_key window_placement panel_key_source panel_list_load panel_item_icons panel_name_text archive_rename_window panel_copy_reload copy_publication_order filesystem_reload address_navigation panel_parent_reload panel_folder_sync native_archive_properties filesystem_comment fs_drives_device_ui namespace_hash native_network_navigation native_persisted_locations address_combo panel_address_keys root_command persisted_location ui_navigation ui_archive_states ui_archive_volumes
            large_list_window worker_window_progress panel_sort_commands panel_refresh_drop panel_refresh_completion panel_rename_state deferred_rename navigation_reload panel_deleted_draw panel_column_menu panel_column_initialization panel_column_schema panel_column_sort panel_column_changes panel_cell_text native_column_workflow panel_item_changed
            archive_reload_command drag_reload_completion panel_creation_path copy_archive_refresh interarchive_recovery panel_load_count panel_activation progress_dialog progress_worker progress_cancel progress_priority archive_progress volume_dialog language_dialog language_standard_buttons fm_settings list_settings_runtime
            language_main_dialog language_focus properties_runtime
            archive_item_open archive_external_edit external_editors_ui language_archive_command overwrite_layout
            alt_streams_command alt_streams_archive_ui fs_provider_command native_archive_ads_copy view_menu)
        get_property(sevenzip_native_ui_environment TEST ${sevenzip_native_ui_test}
            PROPERTY ENVIRONMENT)
        list(TRANSFORM sevenzip_native_ui_environment REPLACE
            "^QT_QPA_PLATFORM=offscreen$" "QT_QPA_PLATFORM=windows")
        set_property(TEST ${sevenzip_native_ui_test} PROPERTY ENVIRONMENT
            "${sevenzip_native_ui_environment}")
        # Native popup/focus checks share the logged-on Windows desktop even
        # when they do not write geometry artifacts.
        set_property(TEST ${sevenzip_native_ui_test} APPEND PROPERTY
            RESOURCE_LOCK sevenzip_native_ui)
    endforeach()
    # Full native UI matrices create 128/82/192 independent windows/workflows.
    # Measured Windows totals are 69/130/165s; allow twice that time rounded
    # up to a minute. Individual state waits and performance guards stay intact.
    set_tests_properties(panel_creation_path PROPERTIES TIMEOUT 180)
    set_tests_properties(interarchive_recovery PROPERTIES TIMEOUT 300)
    set_tests_properties(copy_archive_refresh PROPERTIES TIMEOUT 360)
    # Explicit long-matrix run: at the original caps Windows published
    # 188/137 of 192 Rename rows, 149/102 of 192 External rows,
    # 185 of 288 renderer rows and 91 of 256 interarchive rows.
    # Allow about twice the extrapolated whole-matrix time, rounded up.
    # Individual waits and application deadlines remain unchanged.
    set_tests_properties(native_rename_2 PROPERTIES TIMEOUT 720)
    set_tests_properties(native_rename_301 PROPERTIES TIMEOUT 1080)
    set_tests_properties(native_external_2 PROPERTIES TIMEOUT 660)
    set_tests_properties(native_external_301 PROPERTIES TIMEOUT 960)
    set_tests_properties(native_renderer_failure PROPERTIES TIMEOUT 600)
    set_tests_properties(interarchive_refresh PROPERTIES TIMEOUT 720)
    # Corrected creation reached 122/256 rows in 120s; Copy completed all
    # 98 Qt checks in 173.32s. Apply the same whole-matrix reserve.
    set_tests_properties(panel_creation_publication PROPERTIES TIMEOUT 540)
    set_tests_properties(native_renderer_copy PROPERTIES TIMEOUT 360)
endif()

if(APPLE AND TEST copy_archive_refresh)
    # Cocoa completed 89 of 192 workflows in 120s with no row failures.
    # Apply the same twice-extrapolated full-matrix reserve as Windows:
    # 120 * 192 / 89 * 2, rounded up to a minute. This suite is opt-in slow;
    # individual state waits and application deadlines remain unchanged.
    set_tests_properties(copy_archive_refresh PROPERTIES TIMEOUT 540)
endif()

if(BUILD_TESTING)
    foreach(sevenzip_workflow_suite IN ITEMS developer_workflow hyperv_runner runtime_cache)
        add_test(NAME ${sevenzip_workflow_suite} COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/${sevenzip_workflow_suite}_tests.py")
        set_tests_properties(${sevenzip_workflow_suite} PROPERTIES TIMEOUT 60)
    endforeach()
endif()
include(cmake/SharedWorkflowObjects.cmake)
include(cmake/DeveloperWorkflow.cmake)

# Extract every platform's presentation sources, including those omitted by the
# current platform's target graph and sources shared through object libraries.
file(GLOB_RECURSE sevenzip_translation_sources CONFIGURE_DEPENDS
    src/*.cpp src/*.h src/*.mm)
file(GLOB sevenzip_translation_catalogs CONFIGURE_DEPENDS translations/sevenzip_*.ts)
list(REMOVE_ITEM sevenzip_translation_catalogs "${CMAKE_CURRENT_SOURCE_DIR}/translations/sevenzip_en.ts")
qt_add_translations(sevenzip_filesystem
    SOURCE_TARGETS sevenzip_filesystem
    SOURCES ${sevenzip_translation_sources}
    TS_FILES ${sevenzip_translation_catalogs}
    PLURALS_TS_FILE translations/sevenzip_en.ts
    MERGE_QT_TRANSLATIONS
    QT_TRANSLATION_CATALOGS qtbase
    RESOURCE_PREFIX "/translations"
    LUPDATE_TARGET sevenzip_lupdate
    LRELEASE_TARGET sevenzip_lrelease
    LUPDATE_OPTIONS -no-obsolete -locations relative)
