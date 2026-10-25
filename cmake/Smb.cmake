# Keep the dependency's build rules and source language. Disable unrelated
# transports, authentication providers and utilities in this local-only server.
function(sevenzip_add_smb2)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
    set(BUILD_SHARED_LIBS ON)
    set(ENABLE_EXAMPLES OFF)
    set(ENABLE_UTILS OFF)
    set(ENABLE_LIBKRB5 OFF)
    set(ENABLE_GSSAPI OFF)
    set(ENABLE_LIBDCERPC OFF)
    set(ENABLE_LIBQUIC OFF)
    set(ENABLE_RDMA OFF)
    add_subdirectory(third_party/libsmb2 libsmb2 EXCLUDE_FROM_ALL)
    set_target_properties(smb2 PROPERTIES AUTOMOC OFF AUTOUIC OFF)
endfunction()
sevenzip_add_smb2()

add_library(sevenzip_smb STATIC src/smbreadonlyserver.cpp src/smbreadonlymetadata.cpp src/smbreadonlypending.cpp
    src/smbreadonlyserver.h src/smbreadonlyserver_p.h)
target_include_directories(sevenzip_smb PUBLIC src)
target_link_libraries(sevenzip_smb PUBLIC Qt6::Core PRIVATE smb2)
if(WIN32)
    target_link_libraries(sevenzip_smb PRIVATE ws2_32)
endif()
set_target_properties(sevenzip_smb PROPERTIES AUTOMOC OFF AUTOUIC OFF)

add_library(sevenzip_mount_protocol STATIC src/archivemountdecision.cpp src/archivemountdecision.h
    src/qtnativeclientcontrol.cpp src/qtnativeclientcontrol.h)
qt_add_repc_merged(sevenzip_mount_protocol src/archivemount.rep)
qt_add_repc_merged(sevenzip_mount_protocol src/nativeclient.rep)
target_include_directories(sevenzip_mount_protocol PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(sevenzip_mount_protocol PUBLIC Qt6::RemoteObjects Qt6::Network)
set_target_properties(sevenzip_mount_protocol PROPERTIES AUTOMOC ON AUTOUIC OFF)
if(WIN32)
    target_link_libraries(sevenzip_mount_protocol PRIVATE advapi32)
endif()

if(APPLE OR WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(sevenzip_native_mount src/nativemountmain.cpp src/qtnativemountjob.h)
    if(WIN32)
        target_sources(sevenzip_native_mount PRIVATE src/qtnativemountjob_win.cpp)
    elseif(APPLE)
        target_sources(sevenzip_native_mount PRIVATE src/qtnativemountjob_mac.mm)
        target_link_libraries(sevenzip_native_mount PRIVATE "-framework NetFS" "-framework CoreFoundation")
    else()
        target_sources(sevenzip_native_mount PRIVATE src/qtnativemountjob_linux.cpp)
    endif()
    target_include_directories(sevenzip_native_mount PRIVATE src)
    target_link_libraries(sevenzip_native_mount PRIVATE sevenzip_mount_protocol)
    set_target_properties(sevenzip_native_mount PROPERTIES
        OUTPUT_NAME SevenzipMount AUTOMOC ON AUTOUIC OFF)
endif()

add_executable(sevenzip_smb_server src/smbservermain.cpp)
target_link_libraries(sevenzip_smb_server PRIVATE sevenzip_smb sevenzip_mount_protocol)
set_target_properties(sevenzip_smb_server PROPERTIES
    OUTPUT_NAME SevenzipSmb AUTOMOC OFF AUTOUIC OFF)
if(APPLE)
    set_target_properties(sevenzip_smb_server sevenzip_native_mount PROPERTIES
        INSTALL_RPATH "@loader_path/../Frameworks")
elseif(UNIX)
    set_target_properties(sevenzip_smb_server sevenzip_native_mount PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib")
endif()

if(BUILD_TESTING AND UNIX)
    add_executable(smb2_directory_probe tests/fixtures/libsmb2_directory_probe.c)
    target_link_libraries(smb2_directory_probe PRIVATE smb2)
    target_include_directories(smb2_directory_probe PRIVATE
        third_party/libsmb2/lib third_party/libsmb2/include/smb2
        "${CMAKE_CURRENT_BINARY_DIR}/libsmb2")
    target_compile_definitions(smb2_directory_probe PRIVATE HAVE_CONFIG_H)
    set_target_properties(smb2_directory_probe PROPERTIES
        AUTOMOC OFF AUTOUIC OFF C_STANDARD 11 C_STANDARD_REQUIRED ON)
    add_test(NAME smb2_server_directory COMMAND smb2_directory_probe)
    set_tests_properties(smb2_server_directory PROPERTIES TIMEOUT 15)

endif()
if(BUILD_TESTING)
    add_executable(smb2_mount_probe tests/fixtures/libsmb2_mount_probe.cpp)
    target_link_libraries(smb2_mount_probe PRIVATE sevenzip_smb)
    set_target_properties(smb2_mount_probe PROPERTIES AUTOMOC OFF AUTOUIC OFF)
endif()
if(WIN32)
    foreach(smb_program IN ITEMS sevenzip_smb_server smb2_mount_probe)
        if(TARGET ${smb_program})
            add_custom_target(${smb_program}_smb_runtime
                COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:smb2>
                    "$<TARGET_FILE_DIR:${smb_program}>/$<TARGET_FILE_NAME:smb2>"
                DEPENDS smb2
                VERBATIM)
            add_dependencies(${smb_program} ${smb_program}_smb_runtime)
        endif()
    endforeach()
endif()
