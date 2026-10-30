# Shared checksum providers for native and filesystem consumers.
add_library(sevenzip_crc32c STATIC src/formats/vhdx/VhdxCrc.cpp)
target_include_directories(sevenzip_crc32c PRIVATE ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive)
set_target_properties(sevenzip_crc32c PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON CXX_VISIBILITY_PRESET hidden)
# Portable slicing paths have immutable tables and no lazy dispatch.
add_library(sevenzip_liblzma_crc STATIC
    ${SEVENZIP_DEPENDENCY_DIR}/xz/src/liblzma/check/crc64_fast.c
    ${SEVENZIP_DEPENDENCY_DIR}/xz/src/liblzma/check/crc32_fast.c)
target_include_directories(sevenzip_liblzma_crc PUBLIC ${SEVENZIP_DEPENDENCY_DIR}/xz/src/liblzma/api
    PRIVATE ${SEVENZIP_DEPENDENCY_DIR}/xz/src/common
    ${SEVENZIP_DEPENDENCY_DIR}/xz/src/liblzma/common)
target_compile_definitions(sevenzip_liblzma_crc PUBLIC LZMA_API_STATIC
    PRIVATE HAVE_STDBOOL_H HAVE__BOOL HAVE_STDINT_H HAVE_INTTYPES_H)
if(CMAKE_C_BYTE_ORDER STREQUAL "BIG_ENDIAN")
    target_compile_definitions(sevenzip_liblzma_crc PRIVATE WORDS_BIGENDIAN)
endif()
set_target_properties(sevenzip_liblzma_crc PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 11 C_STANDARD_REQUIRED ON
    C_VISIBILITY_PRESET hidden)
set_source_files_properties(src/checksums/crc64/XzCrc64.c PROPERTIES
    INCLUDE_DIRECTORIES "${SEVENZIP_7ZIP_SOURCE_DIR}/C;${CMAKE_CURRENT_SOURCE_DIR}/third_party")
target_link_libraries(sevenzip_common PUBLIC sevenzip_liblzma_crc)
if(BUILD_TESTING)
    add_executable(linux_checksum_tests tests/crypto/linux_checksum_tests.c src/checksums/linux/crc32-main.c)
    target_link_libraries(linux_checksum_tests PRIVATE sevenzip_liblzma_crc sevenzip_crc32c)
    set_target_properties(linux_checksum_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF
        C_STANDARD 11 C_STANDARD_REQUIRED ON)
    add_test(NAME linux_checksum COMMAND linux_checksum_tests)
    set_tests_properties(linux_checksum PROPERTIES TIMEOUT 15)
    add_test(NAME native_vhdx_crc COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/vhdx/native_vhdx_crc_tests.py"
        "${SEVENZIP_CODEC_RUNTIME_DIR}")
    set_tests_properties(native_vhdx_crc PROPERTIES TIMEOUT 45)
endif()
