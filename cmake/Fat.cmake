# Linux FAT/NLS C code is isolated in a GPL format module.
add_library(sevenzip_fat MODULE
    src/CPP/7zip/Archive/FatHandler.cpp src/exfatmodule.cpp
    src/fs/fat/inode.c src/fs/fat/misc.c src/fs/fat/dir.c
    src/fs/nls/nls_cp437.c src/fs/nls/nls_base.c
    third_party/7zip/CPP/7zip/Archive/Common/ItemNameUtils.cpp
    third_party/7zip/CPP/7zip/Archive/ArchiveExports.cpp
    third_party/7zip/CPP/7zip/Common/StreamUtils.cpp
    src/CPP/7zip/Common/LimitedStreams.cpp
    third_party/7zip/CPP/7zip/Common/ProgressUtils.cpp
    third_party/7zip/CPP/7zip/Common/PropId.cpp
    third_party/7zip/CPP/7zip/Compress/CopyCoder.cpp
    third_party/7zip/CPP/Windows/PropVariant.cpp
    third_party/7zip/CPP/Windows/PropVariantUtils.cpp
    third_party/7zip/CPP/Windows/PropVariantConv.cpp
    third_party/7zip/CPP/Windows/TimeUtils.cpp
    third_party/7zip/CPP/Common/MyWindows.cpp
    third_party/7zip/CPP/Common/MyString.cpp
    third_party/7zip/CPP/Common/MyVector.cpp
    third_party/7zip/CPP/Common/IntToString.cpp
    third_party/7zip/CPP/Common/StringConvert.cpp
    third_party/7zip/CPP/Common/UTFConvert.cpp
    third_party/7zip/C/CpuArch.c third_party/7zip/C/Alloc.c)
target_include_directories(sevenzip_fat PRIVATE src third_party/7zip/CPP
    third_party/7zip/CPP/7zip/Archive)
target_compile_definitions(sevenzip_fat PRIVATE SEVENZIP_LINUX_FAT)
target_link_libraries(sevenzip_fat PRIVATE Threads::Threads)
set_target_properties(sevenzip_fat PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_fat PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_fat PRIVATE src/exfatmodule.def)
    target_link_libraries(sevenzip_fat PRIVATE oleaut32)
endif()
function(sevenzip_deploy_fat)
    add_custom_target(sevenzip_fat_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_fat>"
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats/$<TARGET_FILE_NAME:sevenzip_fat>"
        DEPENDS sevenzip_fat VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_fat_runtime)
endfunction()
if(BUILD_TESTING)
    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
    qt_add_executable(fat_module_tests tests/fat_module_tests.cpp tests/fat_test_support.h)
    set_target_properties(fat_module_tests PROPERTIES AUTOMOC ON)
    target_include_directories(fat_module_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(fat_module_tests PRIVATE sevenzip_panel Qt6::Test)
    target_compile_definitions(fat_module_tests PRIVATE
        "SEVENZIP_FAT_MODULE=\"$<TARGET_FILE:sevenzip_fat>\""
        "SEVENZIP_FAT_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_FAT_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_FAT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/fat_fixture.py\"")
    if(WIN32)
        target_link_libraries(fat_module_tests PRIVATE oleaut32)
    endif()
    add_dependencies(fat_module_tests sevenzip_fat_runtime)
    add_test(NAME fat_module COMMAND fat_module_tests -silent)
    set_tests_properties(fat_module PROPERTIES TIMEOUT 45)
    qt_add_executable(fat_directory_tests tests/fat_directory_tests.cpp tests/fat_test_support.h)
    set_target_properties(fat_directory_tests PROPERTIES AUTOMOC ON)
    target_include_directories(fat_directory_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(fat_directory_tests PRIVATE sevenzip_filesystem Qt6::Test)
    get_target_property(fat_test_definitions fat_module_tests COMPILE_DEFINITIONS)
    target_compile_definitions(fat_directory_tests PRIVATE ${fat_test_definitions})
    if(WIN32)
        target_link_libraries(fat_directory_tests PRIVATE oleaut32)
    endif()
    add_dependencies(fat_directory_tests sevenzip_fat_runtime)
    add_test(NAME fat_directory COMMAND fat_directory_tests -silent)
    set_tests_properties(fat_directory PROPERTIES TIMEOUT 45)
endif()
