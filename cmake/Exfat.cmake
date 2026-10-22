# The GPL Linux-derived reader is a separate 7-Zip format module.
add_library(sevenzip_exfat MODULE
    src/exfathandler.cpp src/exfatmodule.cpp src/exfatreader.c
    src/fs/exfat/dir.c src/fs/exfat/fatent.c src/fs/exfat/super.c
    src/fs/exfat/balloc.c src/fs/exfat/misc.c src/kernel/time/time.c src/lib/hweight.c
    src/lib/string.c
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
target_include_directories(sevenzip_exfat PRIVATE src third_party/7zip/CPP)
target_link_libraries(sevenzip_exfat PRIVATE Threads::Threads)
set_target_properties(sevenzip_exfat PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_exfat PRIVATE "-Wl,-Bsymbolic")
endif()
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(exfat_tests tests/exfat_tests.cpp tests/exfat_fixture.cpp
        third_party/7zip/CPP/Common/MyWindows.cpp
        third_party/7zip/CPP/Windows/PropVariant.cpp
        third_party/7zip/CPP/Common/MyString.cpp
        third_party/7zip/CPP/Common/IntToString.cpp
        third_party/7zip/CPP/Common/StringConvert.cpp
        third_party/7zip/CPP/Common/UTFConvert.cpp
        third_party/7zip/C/Alloc.c)
    target_include_directories(exfat_tests PRIVATE third_party/7zip/CPP)
    set_target_properties(exfat_tests PROPERTIES AUTOMOC ON)
    target_link_libraries(exfat_tests PRIVATE Qt6::Test)
    target_compile_definitions(exfat_tests PRIVATE
        "SEVENZIP_EXFAT_MODULE=\"$<TARGET_FILE:sevenzip_exfat>\""
        "SEVENZIP_EXFAT_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\"")
    if(WIN32)
        target_link_libraries(exfat_tests PRIVATE oleaut32)
    endif()
    add_dependencies(exfat_tests sevenzip_exfat)
    add_test(NAME exfat COMMAND exfat_tests -silent)
    set_tests_properties(exfat PROPERTIES TIMEOUT 45)
    qt_add_executable(exfat_backend_tests tests/exfat_backend_tests.cpp tests/exfat_fixture.cpp)
    set_target_properties(exfat_backend_tests PROPERTIES AUTOMOC ON)
    target_include_directories(exfat_backend_tests PRIVATE src)
    target_link_libraries(exfat_backend_tests PRIVATE sevenzip_panel Qt6::Test)
    target_compile_definitions(exfat_backend_tests PRIVATE
        "SEVENZIP_EXFAT_MODULE=\"$<TARGET_FILE:sevenzip_exfat>\""
        "SEVENZIP_EXFAT_RUNTIME=\"${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime\"")
    add_dependencies(exfat_backend_tests sevenzip_exfat)
    add_test(NAME exfat_backend COMMAND exfat_backend_tests -silent)
    set_tests_properties(exfat_backend PROPERTIES TIMEOUT 45)
endif()
if(WIN32)
    target_sources(sevenzip_exfat PRIVATE src/exfatmodule.def)
    target_link_libraries(sevenzip_exfat PRIVATE oleaut32)
endif()

function(sevenzip_deploy_exfat)
    add_custom_target(sevenzip_exfat_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_exfat>"
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats/$<TARGET_FILE_NAME:sevenzip_exfat>"
        DEPENDS sevenzip_exfat VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_exfat_runtime)
endfunction()
