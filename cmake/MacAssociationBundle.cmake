# Build metadata projection of the exact source association STRINGTABLE100.
# The runtime functional parser remains CCodecIcons::LoadIcons.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${codec_icon_resource}")
file(READ "${codec_icon_resource}" association_resource)
string(REGEX MATCH "100[ \t]+\"([^\"]+)\"" association_line "${association_resource}")
if(NOT association_line)
    message(FATAL_ERROR "Source association STRINGTABLE100 is missing")
endif()
set(association_pairs "${CMAKE_MATCH_1}")
string(REPLACE " " ";" association_pairs "${association_pairs}")
set(SEVENZIP_ASSOCIATION_EXTENSION_TAGS "")
foreach(pair IN LISTS association_pairs)
    if(NOT pair MATCHES "^([a-z0-9]+):[0-9]+$")
        message(FATAL_ERROR "Unsupported source association resource pair: ${pair}")
    endif()
    string(APPEND SEVENZIP_ASSOCIATION_EXTENSION_TAGS "            <string>${CMAKE_MATCH_1}</string>\n")
endforeach()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/resources/macos-bundle.plist.in"
    "${CMAKE_CURRENT_BINARY_DIR}/macos-bundle.plist.in" @ONLY)
# Native bundle metadata needs an ICNS container, while the authoritative source
# image remains the original ICO shared by 7zFM and 7zG. Use macOS's converter.
set(sevenzip_bundle_icon "${CMAKE_CURRENT_BINARY_DIR}/FM.icns")
add_custom_command(OUTPUT "${sevenzip_bundle_icon}"
    COMMAND /usr/bin/sips -s format icns
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/FileManager/FM.ico"
        --out "${sevenzip_bundle_icon}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/UI/FileManager/FM.ico"
    VERBATIM)
set_source_files_properties("${sevenzip_bundle_icon}" PROPERTIES
    GENERATED TRUE MACOSX_PACKAGE_LOCATION Resources)
target_sources(sevenzip_desktop PRIVATE "${sevenzip_bundle_icon}")
set_target_properties(sevenzip_desktop PROPERTIES
    MACOSX_BUNDLE_ICON_FILE FM.icns
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/macos-bundle.plist.in"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.sevenzip-desktop"
    MACOSX_BUNDLE_BUNDLE_NAME "Sevenzip Desktop"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
