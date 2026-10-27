# Static dependencies have no runtime library file for deployment tools to scan.
if(VCPKG_MANIFEST_MODE)
    if(APPLE)
        set(sevenzip_vcpkg_notices_destination Sevenzip.app/Contents/Resources/Notices/vcpkg)
    else()
        set(sevenzip_vcpkg_notices_destination share/licenses/sevenzip-desktop/vcpkg)
    endif()
    file(GLOB sevenzip_vcpkg_notices CONFIGURE_DEPENDS
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/*/copyright")
    foreach(sevenzip_notice IN LISTS sevenzip_vcpkg_notices)
        get_filename_component(sevenzip_notice_directory "${sevenzip_notice}" DIRECTORY)
        get_filename_component(sevenzip_notice_port "${sevenzip_notice_directory}" NAME)
        install(FILES "${sevenzip_notice}"
            DESTINATION "${sevenzip_vcpkg_notices_destination}/${sevenzip_notice_port}")
    endforeach()
endif()
