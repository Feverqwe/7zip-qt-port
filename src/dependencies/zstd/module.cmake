# Official libzstd closure shared by encoders and block-decoder consumers.
file(GLOB zstd_sources CONFIGURE_DEPENDS
    "${SEVENZIP_DEPENDENCY_DIR}/zstd/lib/common/*.c"
    "${SEVENZIP_DEPENDENCY_DIR}/zstd/lib/compress/*.c"
    "${SEVENZIP_DEPENDENCY_DIR}/zstd/lib/decompress/*.c")
add_library(sevenzip_zstd STATIC ${zstd_sources})
target_compile_definitions(sevenzip_zstd PRIVATE ZSTD_DISABLE_ASM)
set_target_properties(sevenzip_zstd PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 11 C_STANDARD_REQUIRED ON)
set_source_files_properties(src/codecs/zstd/ZstdDec.c PROPERTIES
    INCLUDE_DIRECTORIES "${SEVENZIP_7ZIP_SOURCE_DIR}/C")
set_source_files_properties(src/checksums/xxhash/Xxh64.c PROPERTIES
    INCLUDE_DIRECTORIES "${SEVENZIP_7ZIP_SOURCE_DIR}/C")
