# Install an explicit runtime payload; never copy the checkout or build tree.
if(APPLE)
    set(runtime_destination Sevenzip.app/Contents/Resources/7zip)
    set(notices_destination Sevenzip.app/Contents/Resources/Notices)
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
install(TARGETS sevenzip_cramfs LIBRARY DESTINATION "${runtime_destination}/Formats")
install(TARGETS sevenzip_erofs LIBRARY DESTINATION "${runtime_destination}/Formats")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/linux/LICENSES/preferred/GPL-2.0"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/linux/LICENSES/preferred/BSD-2-Clause"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/linux/LICENSES/deprecated/0BSD"
    DESTINATION "${notices_destination}/sevenzip-erofs")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/linux/LICENSES/preferred/GPL-2.0"
    DESTINATION "${notices_destination}/sevenzip-cramfs")
install(TARGETS sevenzip_brotli LIBRARY DESTINATION "${runtime_destination}/Formats")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/brotli/LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/brotli/7zip-zstd-LICENSE.txt"
    DESTINATION "${notices_destination}/brotli")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/linux/LICENSES/preferred/GPL-2.0"
    DESTINATION "${notices_destination}/sevenzip-exfat")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    DESTINATION "${notices_destination}/sevenzip-desktop")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lucide/LICENSE"
    DESTINATION "${notices_destination}/lucide")
if(APPLE)
    qt_generate_deploy_app_script(TARGET sevenzip_desktop OUTPUT_SCRIPT unix_deploy_script
        NO_TRANSLATIONS DEPLOY_TOOL_OPTIONS
        "-executable=Sevenzip.app/Contents/MacOS/SevenzipDeviceReader")
else()
    qt_generate_deploy_app_script(TARGET sevenzip_desktop OUTPUT_SCRIPT unix_deploy_script
        NO_TRANSLATIONS)
endif()
install(SCRIPT "${unix_deploy_script}")
