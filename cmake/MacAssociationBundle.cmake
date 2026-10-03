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
set_target_properties(sevenzip_desktop PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/macos-bundle.plist.in"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.sevenzip-desktop"
    MACOSX_BUNDLE_BUNDLE_NAME "Sevenzip Desktop"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
