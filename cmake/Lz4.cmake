# One official raw decoder for Frame, legacy LZ4 and filesystem consumers.
add_library(sevenzip_lz4 STATIC third_party/lz4/lib/lz4.c)
target_include_directories(sevenzip_lz4 PUBLIC third_party/lz4/lib)
set_target_properties(sevenzip_lz4 PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 99 C_STANDARD_REQUIRED ON)

add_library(sevenzip_lz4_frame_core STATIC
    src/lz4frameio.c
    third_party/lz4/lib/lz4hc.c third_party/lz4/lib/lz4frame.c third_party/lz4/lib/xxhash.c)
target_link_libraries(sevenzip_lz4_frame_core PUBLIC sevenzip_lz4)
set_target_properties(sevenzip_lz4_frame_core PROPERTIES AUTOMOC OFF AUTOUIC OFF
    POSITION_INDEPENDENT_CODE ON C_STANDARD 99 C_STANDARD_REQUIRED ON)
