# Match the application's deployment floor and SDK for dependency builds.
set(VCPKG_OSX_DEPLOYMENT_TARGET 14.4)
if(DEFINED ENV{SDKROOT} AND NOT "$ENV{SDKROOT}" STREQUAL "")
    set(VCPKG_OSX_SYSROOT "$ENV{SDKROOT}")
else()
    execute_process(COMMAND xcrun --show-sdk-path
        OUTPUT_VARIABLE VCPKG_OSX_SYSROOT OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
endif()
file(REAL_PATH "${VCPKG_OSX_SYSROOT}" VCPKG_OSX_SYSROOT)
if(PORT MATCHES "^qt")
    execute_process(COMMAND xcode-select --print-path
        OUTPUT_VARIABLE sevenzip_developer_dir OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    if(sevenzip_developer_dir MATCHES "/CommandLineTools$")
        # Desktop Qt can use CLT; retain SDK checks without requiring Xcode.app.
        list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS -DQT_NO_XCODE_MIN_VERSION_CHECK=ON)
    endif()
endif()
list(APPEND VCPKG_ENV_PASSTHROUGH SDKROOT
    CMAKE_C_COMPILER_LAUNCHER CMAKE_CXX_COMPILER_LAUNCHER CMAKE_OBJCXX_COMPILER_LAUNCHER)
list(APPEND VCPKG_HASH_ADDITIONAL_FILES "${CMAKE_CURRENT_LIST_FILE}"
    "${VCPKG_OSX_SYSROOT}/SDKSettings.json")
