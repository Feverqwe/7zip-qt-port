# Install an explicit runtime payload; never copy the checkout or build tree.
if(APPLE)
    set(runtime_destination sevenzip_desktop.app/Contents/Resources/7zip)
    set(notices_destination sevenzip_desktop.app/Contents/Resources/Notices)
    install(TARGETS sevenzip_desktop BUNDLE DESTINATION .)
else()
    set(runtime_destination bin)
    set(notices_destination share/licenses/sevenzip-desktop)
    install(TARGETS sevenzip_desktop RUNTIME DESTINATION bin)
    install(PROGRAMS "$<TARGET_FILE_DIR:sevenzip_desktop>/7zFM"
        "$<TARGET_FILE_DIR:sevenzip_desktop>/7zG" DESTINATION bin)
endif()
install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/7z"
    DESTINATION "${runtime_destination}")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/7z.so"
    "${CMAKE_CURRENT_SOURCE_DIR}/build/sfx/7zCon.sfx"
    "${CMAKE_CURRENT_SOURCE_DIR}/build/sfx/7zCon.sfx.json"
    DESTINATION "${runtime_destination}")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/LICENSE.txt"
    DESTINATION "${notices_destination}/7zip")
install(TARGETS sevenzip_exfat LIBRARY DESTINATION "${runtime_destination}/Formats")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/linux/LICENSES/preferred/GPL-2.0"
    DESTINATION "${notices_destination}/sevenzip-exfat")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    DESTINATION "${notices_destination}/sevenzip-desktop")
qt_generate_deploy_app_script(TARGET sevenzip_desktop OUTPUT_SCRIPT unix_deploy_script
    NO_TRANSLATIONS)
install(SCRIPT "${unix_deploy_script}")
