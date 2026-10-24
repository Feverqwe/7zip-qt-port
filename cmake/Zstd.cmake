# Official libzstd closure shared by encoders and block-decoder consumers.
file(GLOB zstd_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zstd/lib/common/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zstd/lib/compress/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zstd/lib/decompress/*.c")
add_library(sevenzip_zstd STATIC ${zstd_sources})
target_compile_definitions(sevenzip_zstd PRIVATE ZSTD_DISABLE_ASM)
set_target_properties(sevenzip_zstd PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 11 C_STANDARD_REQUIRED ON)
set_source_files_properties(src/C/ZstdDec.c PROPERTIES
    INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/C")
