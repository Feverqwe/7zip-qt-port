# Share the existing pinned liblzma dependency with SDK/native CRC64 consumers.
# The portable slice-by-four path has immutable tables and no lazy dispatch.
add_library(sevenzip_liblzma_crc64 STATIC
    third_party/xz/src/liblzma/check/crc64_fast.c)
target_include_directories(sevenzip_liblzma_crc64 PRIVATE
    third_party/xz/src/liblzma/api third_party/xz/src/common
    third_party/xz/src/liblzma/common)
target_compile_definitions(sevenzip_liblzma_crc64 PRIVATE
    LZMA_API_STATIC HAVE_STDBOOL_H HAVE__BOOL HAVE_STDINT_H HAVE_INTTYPES_H)
if(CMAKE_C_BYTE_ORDER STREQUAL "BIG_ENDIAN")
    target_compile_definitions(sevenzip_liblzma_crc64 PRIVATE WORDS_BIGENDIAN)
endif()
set_target_properties(sevenzip_liblzma_crc64 PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 11 C_STANDARD_REQUIRED ON
    C_VISIBILITY_PRESET hidden)
set_source_files_properties(src/C/XzCrc64.c PROPERTIES
    INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/C;${CMAKE_CURRENT_SOURCE_DIR}/third_party")
target_link_libraries(sevenzip_common PUBLIC sevenzip_liblzma_crc64)
