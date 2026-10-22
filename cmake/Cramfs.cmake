# Linux-derived C readers remain in a separate GPL format module.
add_library(sevenzip_cramfs MODULE
    src/CPP/7zip/Archive/CramfsHandler.cpp src/exfatmodule.cpp
    src/cramfsreader.c src/fs/cramfs/inode.c
    third_party/7zip/CPP/7zip/Archive/ArchiveExports.cpp
    third_party/7zip/CPP/7zip/Common/StreamUtils.cpp
    third_party/7zip/CPP/7zip/Common/StreamObjects.cpp
    src/CPP/7zip/Common/LimitedStreams.cpp
    third_party/7zip/CPP/7zip/Common/InBuffer.cpp
    third_party/7zip/CPP/7zip/Common/OutBuffer.cpp
    third_party/7zip/CPP/7zip/Common/ProgressUtils.cpp
    third_party/7zip/CPP/7zip/Common/PropId.cpp
    third_party/7zip/CPP/7zip/Compress/CopyCoder.cpp
    third_party/7zip/CPP/7zip/Compress/ZlibDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/DeflateDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/BitlDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/LzOutWindow.cpp
    third_party/7zip/CPP/Windows/PropVariant.cpp
    third_party/7zip/CPP/Windows/PropVariantUtils.cpp
    third_party/7zip/CPP/Windows/PropVariantConv.cpp
    third_party/7zip/CPP/Windows/TimeUtils.cpp
    third_party/7zip/CPP/Common/CRC.cpp
    third_party/7zip/CPP/Common/MyWindows.cpp
    third_party/7zip/CPP/Common/MyString.cpp
    third_party/7zip/CPP/Common/MyVector.cpp
    third_party/7zip/CPP/Common/IntToString.cpp
    third_party/7zip/CPP/Common/StringConvert.cpp
    third_party/7zip/CPP/Common/UTFConvert.cpp
    third_party/7zip/C/7zCrc.c third_party/7zip/C/7zCrcOpt.c
    third_party/7zip/C/CpuArch.c third_party/7zip/C/Alloc.c third_party/7zip/C/LzmaDec.c)
target_include_directories(sevenzip_cramfs PRIVATE src third_party/7zip/CPP
    third_party/7zip/CPP/7zip/Archive)
target_compile_definitions(sevenzip_cramfs PRIVATE SEVENZIP_LINUX_CRAMFS)
target_link_libraries(sevenzip_cramfs PRIVATE Threads::Threads)
set_target_properties(sevenzip_cramfs PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_cramfs PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_cramfs PRIVATE src/exfatmodule.def)
    target_link_libraries(sevenzip_cramfs PRIVATE oleaut32)
endif()
function(sevenzip_deploy_cramfs)
    add_custom_target(sevenzip_cramfs_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_cramfs>"
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats/$<TARGET_FILE_NAME:sevenzip_cramfs>"
        DEPENDS sevenzip_cramfs VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_cramfs_runtime)
endfunction()
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
    qt_add_executable(cramfs_module_tests tests/cramfs_module_tests.cpp)
    set_target_properties(cramfs_module_tests PROPERTIES AUTOMOC ON)
    target_include_directories(cramfs_module_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(cramfs_module_tests PRIVATE sevenzip_panel Qt6::Test)
    target_compile_definitions(cramfs_module_tests PRIVATE
        "SEVENZIP_CRAMFS_MODULE=\"$<TARGET_FILE:sevenzip_cramfs>\""
        "SEVENZIP_CRAMFS_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\""
        "SEVENZIP_CRAMFS_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_CRAMFS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/cramfs_fixture.py\"")
    if(WIN32)
        target_link_libraries(cramfs_module_tests PRIVATE oleaut32)
    endif()
    add_dependencies(cramfs_module_tests sevenzip_cramfs_runtime)
    add_test(NAME cramfs_module COMMAND cramfs_module_tests -silent)
    set_tests_properties(cramfs_module PROPERTIES TIMEOUT 45)
endif()
