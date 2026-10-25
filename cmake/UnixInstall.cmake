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
if(sevenzip_openssl_usable)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/openssl/LICENSE.txt"
        DESTINATION "${notices_destination}/openssl")
endif()
