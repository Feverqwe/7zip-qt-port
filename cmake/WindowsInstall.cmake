# Qt and the retained Windows entrypoints share one deployment directory.
# Registration is a separate Menu-page operation; installing files never writes
# associations or registers the extension on the build machine.
set(SEVENZIP_WINDOWS_RUNTIME "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime" CACHE PATH
    "Official pinned Windows runtime prepared by scripts/build-codec-runtime.ps1")
string(JSON windows_runtime_repository GET "${sfx_baseline_json}" repository)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64)$")
    set(windows_runtime_architecture arm64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(windows_runtime_architecture x86_64)
else()
    set(windows_runtime_architecture x86)
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyWindowsRuntime.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/VerifyWindowsRuntime.cmake" @ONLY)
install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/VerifyWindowsRuntime.cmake")
install(TARGETS sevenzip_desktop sevenzip_shell RUNTIME DESTINATION bin LIBRARY DESTINATION bin)
install(FILES "$<TARGET_FILE:sevenzip_desktop>" DESTINATION bin RENAME 7zFM.exe)
install(FILES "$<TARGET_FILE:sevenzip_desktop>" DESTINATION bin RENAME 7zG.exe)
install(FILES "${SEVENZIP_WINDOWS_RUNTIME}/7z.exe" "${SEVENZIP_WINDOWS_RUNTIME}/7z.dll"
    DESTINATION bin)
install(FILES "${SEVENZIP_WINDOWS_RUNTIME}/runtime.json"
    DESTINATION share/sevenzip-desktop)
install(FILES "${SEVENZIP_WINDOWS_RUNTIME}/LICENSE.txt"
    DESTINATION share/licenses/sevenzip-desktop/7zip)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    DESTINATION share/licenses/sevenzip-desktop/sevenzip-desktop)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/build/sfx/7z.sfx"
    "${CMAKE_CURRENT_SOURCE_DIR}/build/sfx/7z.sfx.json" DESTINATION bin)
qt_generate_deploy_app_script(TARGET sevenzip_desktop OUTPUT_SCRIPT windows_deploy_script
    NO_TRANSLATIONS)
install(SCRIPT "${windows_deploy_script}")
