# GRUB's GPL-3.0-or-later reader stays in its own archive-only module.
add_library(sevenzip_f2fs_core STATIC src/fs/f2fs/f2fs.c)
target_include_directories(sevenzip_f2fs_core PRIVATE src)
set_target_properties(sevenzip_f2fs_core PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 11 C_STANDARD_REQUIRED ON)
add_library(sevenzip_f2fs MODULE
    src/f2fshandler.cpp src/f2fsarchivereader.cpp src/exfatmodule.cpp
    third_party/7zip/CPP/7zip/Archive/ArchiveExports.cpp
    third_party/7zip/CPP/7zip/Common/StreamUtils.cpp
    third_party/7zip/CPP/7zip/Common/PropId.cpp
    third_party/7zip/CPP/Windows/PropVariant.cpp
    third_party/7zip/CPP/Common/MyWindows.cpp
    third_party/7zip/CPP/Common/MyString.cpp
    third_party/7zip/CPP/Common/MyVector.cpp
    third_party/7zip/CPP/Common/IntToString.cpp
    third_party/7zip/CPP/Common/StringConvert.cpp
    third_party/7zip/CPP/Common/UTFConvert.cpp)
target_include_directories(sevenzip_f2fs PRIVATE src third_party/7zip/CPP
    third_party/7zip/CPP/7zip/Archive)
target_link_libraries(sevenzip_f2fs PRIVATE sevenzip_f2fs_core Threads::Threads)
set_target_properties(sevenzip_f2fs PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX "")
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_f2fs PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_f2fs PRIVATE src/exfatmodule.def)
    target_link_libraries(sevenzip_f2fs PRIVATE oleaut32)
endif()
function(sevenzip_deploy_f2fs)
    add_custom_target(sevenzip_f2fs_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_f2fs>"
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats/$<TARGET_FILE_NAME:sevenzip_f2fs>"
        DEPENDS sevenzip_f2fs VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_f2fs_runtime)
endfunction()
if(BUILD_TESTING)
    qt_add_executable(f2fs_module_tests tests/f2fs_module_tests.cpp
        third_party/7zip/CPP/Windows/PropVariant.cpp
        third_party/7zip/CPP/Common/MyWindows.cpp
        third_party/7zip/CPP/Common/MyString.cpp
        third_party/7zip/CPP/Common/MyVector.cpp
        third_party/7zip/CPP/Common/IntToString.cpp
        third_party/7zip/CPP/Common/StringConvert.cpp
        third_party/7zip/CPP/Common/UTFConvert.cpp)
    target_include_directories(f2fs_module_tests PRIVATE src third_party/7zip/CPP)
    set_target_properties(f2fs_module_tests PROPERTIES AUTOMOC ON)
    target_link_libraries(f2fs_module_tests PRIVATE Qt6::Test)
    target_compile_definitions(f2fs_module_tests PRIVATE
        "SEVENZIP_F2FS_MODULE=\"$<TARGET_FILE:sevenzip_f2fs>\""
        "SEVENZIP_F2FS_FIXTURES=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/f2fs\"")
    if(WIN32)
        target_link_libraries(f2fs_module_tests PRIVATE oleaut32)
    endif()
    add_dependencies(f2fs_module_tests sevenzip_f2fs)
    add_test(NAME f2fs_module COMMAND f2fs_module_tests -silent)
    set_tests_properties(f2fs_module PROPERTIES TIMEOUT 45)
    qt_add_executable(f2fs_native_tests tests/f2fs_native_tests.cpp)
    set_target_properties(f2fs_native_tests PROPERTIES AUTOMOC ON)
    target_include_directories(f2fs_native_tests PRIVATE src tests)
    target_link_libraries(f2fs_native_tests PRIVATE sevenzip_filesystem Qt6::Test)
    target_compile_definitions(f2fs_native_tests PRIVATE
        "SEVENZIP_F2FS_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_F2FS_FIXTURES=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/f2fs\"")
    add_dependencies(f2fs_native_tests sevenzip_f2fs_runtime)
    add_test(NAME f2fs_native COMMAND f2fs_native_tests -silent)
    set_tests_properties(f2fs_native PROPERTIES TIMEOUT 45)
endif()
