# Linux FAT/NLS C code is isolated in a GPL format module.
add_library(sevenzip_fat MODULE
    src/formats/fat/FatHeader.cpp
    src/formats/fat/FatHandler.cpp src/formats/common/moduleexports.cpp
    src/formats/fat/inode.c src/formats/fat/misc.c src/formats/fat/dir.c
    src/text/nls/nls_base.c src/text/nls/fatnames.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Common/ItemNameUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/ArchiveExports.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/StreamUtils.cpp
    src/archive/streams/LimitedStreams.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/ProgressUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/PropId.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/CopyCoder.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/PropVariant.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/PropVariantUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/PropVariantConv.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/TimeUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyWindows.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyString.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyVector.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/IntToString.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/StringConvert.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/UTFConvert.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/CpuArch.c ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Alloc.c)
target_include_directories(sevenzip_fat PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive)
target_link_libraries(sevenzip_fat PRIVATE Threads::Threads sevenzip_nls_system)
set_target_properties(sevenzip_fat PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_fat PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_fat PRIVATE src/formats/common/moduleexports.def)
    target_link_libraries(sevenzip_fat PRIVATE oleaut32)
endif()
function(sevenzip_deploy_fat)
    add_custom_target(sevenzip_fat_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_fat>"
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats/$<TARGET_FILE_NAME:sevenzip_fat>"
        DEPENDS sevenzip_fat VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_fat_runtime)
endfunction()
if(BUILD_TESTING)
    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
    qt_add_executable(fat_module_tests tests/formats/fat/fat_module_tests.cpp tests/formats/fat/support/fat_test_support.h)
    set_target_properties(fat_module_tests PROPERTIES AUTOMOC ON)
    target_include_directories(fat_module_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(fat_module_tests PRIVATE sevenzip_panel Qt6::Test)
    target_compile_definitions(fat_module_tests PRIVATE
        "SEVENZIP_FAT_MODULE=\"$<TARGET_FILE:sevenzip_fat>\""
        "SEVENZIP_FAT_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_FAT_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_FAT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/fat/support/fat_fixture.py\"")
    if(WIN32)
        target_link_libraries(fat_module_tests PRIVATE oleaut32)
    endif()
    add_dependencies(fat_module_tests sevenzip_fat_runtime)
    add_test(NAME fat_module COMMAND fat_module_tests -silent)
    set_tests_properties(fat_module PROPERTIES TIMEOUT 45)
    qt_add_executable(fat_directory_tests tests/formats/fat/fat_directory_tests.cpp tests/formats/fat/support/fat_test_support.h)
    set_target_properties(fat_directory_tests PROPERTIES AUTOMOC ON)
    target_include_directories(fat_directory_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(fat_directory_tests PRIVATE sevenzip_filesystem Qt6::Test)
    get_target_property(fat_test_definitions fat_module_tests COMPILE_DEFINITIONS)
    target_compile_definitions(fat_directory_tests PRIVATE ${fat_test_definitions})
    if(WIN32)
        target_link_libraries(fat_directory_tests PRIVATE oleaut32)
    endif()
    add_dependencies(fat_directory_tests sevenzip_fat_runtime)
    add_test(NAME fat_directory COMMAND fat_directory_tests -silent)
    set_tests_properties(fat_directory PROPERTIES TIMEOUT 45)
    qt_add_executable(fat_stream_tests tests/formats/fat/fat_stream_tests.cpp tests/formats/fat/support/fat_test_support.h)
    set_target_properties(fat_stream_tests PROPERTIES AUTOMOC ON)
    target_include_directories(fat_stream_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(fat_stream_tests PRIVATE sevenzip_filesystem Qt6::Test)
    target_compile_definitions(fat_stream_tests PRIVATE ${fat_test_definitions})
    if(WIN32)
        target_link_libraries(fat_stream_tests PRIVATE oleaut32)
    endif()
    add_dependencies(fat_stream_tests sevenzip_fat_runtime)
    add_test(NAME fat_stream COMMAND fat_stream_tests -silent)
    set_tests_properties(fat_stream PROPERTIES TIMEOUT 45)
    qt_add_executable(fat_maximum_tests tests/formats/fat/fat_maximum_tests.cpp tests/formats/fat/support/fat_test_support.h)
    set_target_properties(fat_maximum_tests PROPERTIES AUTOMOC ON)
    target_include_directories(fat_maximum_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(fat_maximum_tests PRIVATE sevenzip_filesystem Qt6::Test)
    target_compile_definitions(fat_maximum_tests PRIVATE ${fat_test_definitions})
    if(WIN32)
        target_link_libraries(fat_maximum_tests PRIVATE oleaut32)
    endif()
    add_dependencies(fat_maximum_tests sevenzip_fat_runtime)
    add_test(NAME fat_maximum COMMAND fat_maximum_tests -silent)
    set_tests_properties(fat_maximum PROPERTIES TIMEOUT 180 PROCESSORS 4)
    qt_add_executable(fat_table_tests tests/formats/fat/fat_table_tests.cpp tests/formats/fat/support/fat_test_support.h)
    set_target_properties(fat_table_tests PROPERTIES AUTOMOC ON)
    target_include_directories(fat_table_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(fat_table_tests PRIVATE sevenzip_filesystem Qt6::Test)
    target_compile_definitions(fat_table_tests PRIVATE ${fat_test_definitions})
    if(WIN32)
        target_link_libraries(fat_table_tests PRIVATE oleaut32)
    endif()
    add_dependencies(fat_table_tests sevenzip_fat_runtime)
    add_test(NAME fat_table COMMAND fat_table_tests -silent)
    set_tests_properties(fat_table PROPERTIES TIMEOUT 45)
    qt_add_executable(fat_table_prompt_tests tests/formats/fat/fat_table_tests.cpp tests/formats/fat/support/fat_test_support.h
        tests/ui/support/layout_inspection.cpp)
    set_target_properties(fat_table_prompt_tests PROPERTIES AUTOMOC ON)
    target_include_directories(fat_table_prompt_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(fat_table_prompt_tests PRIVATE sevenzip_location Qt6::Test)
    target_compile_definitions(fat_table_prompt_tests PRIVATE ${fat_test_definitions}
        SEVENZIP_FAT_TABLE_PROMPT)
    if(WIN32)
        target_link_libraries(fat_table_prompt_tests PRIVATE oleaut32)
    endif()
    add_dependencies(fat_table_prompt_tests sevenzip_fat_runtime)
    add_test(NAME fat_table_prompt COMMAND fat_table_prompt_tests -silent limitedTableConsent)
    set_tests_properties(fat_table_prompt PROPERTIES TIMEOUT 45 RESOURCE_LOCK sevenzip_ui_artifacts
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    if(APPLE)
        set_property(TEST fat_table_prompt PROPERTY ENVIRONMENT
            "QT_QPA_PLATFORM=cocoa;SEVENZIP_UI_REPORT_DIR=${CMAKE_BINARY_DIR}/ui-reports")
    endif()
    # Compound is the other handler using the shared CClusterInStream reader.
    add_test(NAME compound_stream COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/archive/compound_stream_tests.py
        ${SEVENZIP_CODEC_RUNTIME_DIR})
    set_tests_properties(compound_stream PROPERTIES TIMEOUT 45)
endif()

# The GPL reader sources use their own upstream-shaped include root.
target_include_directories(sevenzip_fat BEFORE PRIVATE ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP ${SEVENZIP_7ZIP_SOURCE_DIR})
