# GPLv3 GRUB read paths are isolated from GPLv2-only Linux filesystem modules.
add_library(sevenzip_xfs_core STATIC src/grub/fs/xfs.c src/grub/fs/fshelp.c
    src/xfsreader.c src/xfschecksum.cpp)
target_include_directories(sevenzip_xfs_core PUBLIC src)
target_link_libraries(sevenzip_xfs_core PUBLIC sevenzip_crc32c)
set_target_properties(sevenzip_xfs_core PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 11 C_STANDARD_REQUIRED ON)
add_library(sevenzip_xfs MODULE src/xfshandler.cpp src/xfsarchivereader.cpp src/exfatmodule.cpp
    third_party/7zip/CPP/7zip/Archive/ArchiveExports.cpp
    third_party/7zip/CPP/7zip/Common/StreamUtils.cpp
    third_party/7zip/CPP/7zip/Common/PropId.cpp
    third_party/7zip/CPP/Windows/PropVariant.cpp
    third_party/7zip/CPP/Windows/TimeUtils.cpp
    third_party/7zip/CPP/Common/MyWindows.cpp
    third_party/7zip/CPP/Common/MyString.cpp
    third_party/7zip/CPP/Common/IntToString.cpp
    third_party/7zip/CPP/Common/StringConvert.cpp
    third_party/7zip/CPP/Common/UTFConvert.cpp
    third_party/7zip/C/Alloc.c)
target_include_directories(sevenzip_xfs PRIVATE src third_party/7zip/CPP)
target_link_libraries(sevenzip_xfs PRIVATE sevenzip_xfs_core Threads::Threads)
set_target_properties(sevenzip_xfs PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX "")
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_xfs PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_xfs PRIVATE src/exfatmodule.def)
    target_link_libraries(sevenzip_xfs PRIVATE oleaut32)
endif()
function(sevenzip_deploy_xfs)
    add_custom_target(sevenzip_xfs_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_xfs>"
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats/$<TARGET_FILE_NAME:sevenzip_xfs>"
        DEPENDS sevenzip_xfs VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_xfs_runtime)
endfunction()
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    qt_add_executable(xfs_module_tests tests/xfs_module_tests.cpp
        third_party/7zip/CPP/Common/MyWindows.cpp
        third_party/7zip/CPP/Windows/PropVariant.cpp
        third_party/7zip/CPP/Common/MyString.cpp
        third_party/7zip/CPP/Common/IntToString.cpp
        third_party/7zip/CPP/Common/StringConvert.cpp
        third_party/7zip/CPP/Common/UTFConvert.cpp
        third_party/7zip/C/Alloc.c)
    target_include_directories(xfs_module_tests PRIVATE third_party/7zip/CPP)
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
    qt_add_executable(xfs_backend_tests tests/xfs_backend_tests.cpp)
    set_target_properties(xfs_backend_tests PROPERTIES AUTOMOC ON)
    target_include_directories(xfs_backend_tests PRIVATE src)
    target_link_libraries(xfs_backend_tests PRIVATE sevenzip_panel Qt6::Test)
    target_compile_definitions(xfs_backend_tests PRIVATE
        "SEVENZIP_XFS_MODULE=\"$<TARGET_FILE:sevenzip_xfs>\""
        "SEVENZIP_XFS_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_XFS_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_XFS_FIXTURES=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/xfs\"")
    add_dependencies(xfs_backend_tests sevenzip_xfs_runtime)
    add_test(NAME xfs_backend COMMAND xfs_backend_tests -silent)
    set_tests_properties(xfs_backend PROPERTIES TIMEOUT 60)
endif()
