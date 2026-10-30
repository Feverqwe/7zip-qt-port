if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 23)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    include(cmake/MacosSdkIncludes.cmake)
    if(SEVENZIP_ENABLE_SANITIZERS)
        foreach(sevenzip_config DEBUG RELWITHDEBINFO)
            string(APPEND CMAKE_OBJCXX_FLAGS_${sevenzip_config}
                " -fsanitize=address,undefined -fno-omit-frame-pointer")
        endforeach()
    endif()
    add_library(sevenzip_device_access STATIC
        src/devices/deviceaccess.h src/devices/deviceaccess_mac.h src/devices/deviceaccess_mac.mm
        src/devices/deviceaccess_posix.cpp src/devices/deviceaccess_posix.h
        src/devices/deviceauthclient_mac.cpp src/devices/deviceauthsocket_mac.h)
    target_include_directories(sevenzip_device_access PUBLIC src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(sevenzip_device_access PUBLIC Qt6::Core
        "-framework DiskArbitration" "-framework IOKit" "-framework Foundation")
    set_target_properties(sevenzip_device_access PROPERTIES AUTOMOC OFF AUTOUIC OFF)
    add_executable(sevenzip_device_reader src/devices/devicereader_mac.cpp)
    target_link_libraries(sevenzip_device_reader PRIVATE sevenzip_device_access)
    set_target_properties(sevenzip_device_reader PROPERTIES
        OUTPUT_NAME SevenzipDeviceReader AUTOMOC OFF AUTOUIC OFF)
elseif(WIN32)
    add_library(sevenzip_device_access STATIC
        src/devices/deviceaccess.h src/devices/deviceaccess_win.h src/devices/deviceaccess_win.cpp
        src/devices/devicehandle_win.cpp src/devices/deviceauthclient_win.cpp src/devices/devicepipe_win.cpp src/devices/devicepipe_win.h)
    target_include_directories(sevenzip_device_access PUBLIC src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(sevenzip_device_access PUBLIC Qt6::Core setupapi cfgmgr32 advapi32 ole32 shell32)
    set_target_properties(sevenzip_device_access PROPERTIES AUTOMOC OFF AUTOUIC OFF)
    qt_add_executable(sevenzip_device_reader WIN32 src/devices/devicereader_win.cpp)
    target_link_libraries(sevenzip_device_reader PRIVATE sevenzip_device_access)
    set_target_properties(sevenzip_device_reader PROPERTIES
        OUTPUT_NAME SevenzipDeviceReader AUTOMOC OFF AUTOUIC OFF)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_library(sevenzip_device_access STATIC
        src/devices/deviceaccess.h src/devices/deviceaccess_linux.cpp
        src/devices/deviceaccess_posix.cpp src/devices/deviceaccess_posix.h)
    target_include_directories(sevenzip_device_access PUBLIC src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(sevenzip_device_access PUBLIC Qt6::Core Qt6::DBus)
    set_target_properties(sevenzip_device_access PROPERTIES AUTOMOC OFF AUTOUIC OFF)
endif()
