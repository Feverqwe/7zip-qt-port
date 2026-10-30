# GPLv3 GRUB read paths are isolated from GPLv2-only Linux filesystem modules.
add_library(sevenzip_xfs_core STATIC src/dependencies/grub/include/grub/fs/xfs.c src/dependencies/grub/include/grub/fs/fshelp.c
    src/formats/xfs/xfsreader.c src/formats/xfs/xfschecksum.cpp)
target_include_directories(sevenzip_xfs_core PUBLIC src)
target_link_libraries(sevenzip_xfs_core PUBLIC sevenzip_crc32c)
set_target_properties(sevenzip_xfs_core PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 11 C_STANDARD_REQUIRED ON)
add_library(sevenzip_xfs MODULE src/formats/xfs/xfshandler.cpp src/formats/xfs/xfsarchivereader.cpp src/formats/common/moduleexports.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/ArchiveExports.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/StreamUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/PropId.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/PropVariant.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/TimeUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyWindows.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyString.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/IntToString.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/StringConvert.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/UTFConvert.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Alloc.c)
target_include_directories(sevenzip_xfs PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
target_link_libraries(sevenzip_xfs PRIVATE Threads::Threads)
set_target_properties(sevenzip_xfs PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX "")
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_xfs PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_xfs PRIVATE src/formats/common/moduleexports.def)
    target_link_libraries(sevenzip_xfs PRIVATE oleaut32)
endif()
function(sevenzip_deploy_xfs)
    add_custom_target(sevenzip_xfs_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_xfs>"
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats/$<TARGET_FILE_NAME:sevenzip_xfs>"
        DEPENDS sevenzip_xfs VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_xfs_runtime)
endfunction()
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    qt_add_executable(xfs_module_tests tests/formats/xfs/xfs_module_tests.cpp
        ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyWindows.cpp
        ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/PropVariant.cpp
        ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyString.cpp
        ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/IntToString.cpp
        ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/StringConvert.cpp
        ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/UTFConvert.cpp
        ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Alloc.c)
    target_include_directories(xfs_module_tests PRIVATE ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    set_target_properties(xfs_module_tests PROPERTIES AUTOMOC ON)
    target_link_libraries(xfs_module_tests PRIVATE Qt6::Test)
    target_compile_definitions(xfs_module_tests PRIVATE
        "SEVENZIP_XFS_MODULE=\"$<TARGET_FILE:sevenzip_xfs>\""
        "SEVENZIP_XFS_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_XFS_FIXTURES=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/xfs\"")
    if(WIN32)
        target_link_libraries(xfs_module_tests PRIVATE oleaut32)
    endif()
    add_dependencies(xfs_module_tests sevenzip_xfs_runtime)
    add_test(NAME xfs_module COMMAND xfs_module_tests -silent)
    set_tests_properties(xfs_module PROPERTIES TIMEOUT 45)
    qt_add_executable(xfs_backend_tests tests/formats/xfs/xfs_backend_tests.cpp)
    set_target_properties(xfs_backend_tests PROPERTIES AUTOMOC ON)
    target_include_directories(xfs_backend_tests PRIVATE src)
    target_link_libraries(xfs_backend_tests PRIVATE sevenzip_panel Qt6::Test)
    target_compile_definitions(xfs_backend_tests PRIVATE
        "SEVENZIP_XFS_MODULE=\"$<TARGET_FILE:sevenzip_xfs>\""
        "SEVENZIP_XFS_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_XFS_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_XFS_FIXTURES=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/xfs\"")
    add_dependencies(xfs_backend_tests sevenzip_xfs_runtime)
    add_test(NAME xfs_backend COMMAND xfs_backend_tests -silent)
    set_tests_properties(xfs_backend PROPERTIES TIMEOUT 60)
endif()
