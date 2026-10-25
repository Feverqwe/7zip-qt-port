# Qt and the retained Windows entrypoints share one deployment directory.
# Registration is a separate Menu-page operation; installing files never writes
# associations or registers the extension on the build machine.
set(SEVENZIP_WINDOWS_RUNTIME "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime" CACHE PATH
    "Official pinned Windows runtime prepared by scripts/build-codec-runtime.ps1")
if(sevenzip_openssl_usable)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/openssl/LICENSE.txt"
        DESTINATION "share/licenses/sevenzip-desktop/openssl")
endif()
