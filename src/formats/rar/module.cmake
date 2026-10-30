# Original RAR5/RAR7 decoder and handler support. The handler itself remains
# the single original Rar5Handler.cpp in the forced hash/arc registration object.
# Baseline: docs/upstream-baseline.json.
add_library(sevenzip_rar STATIC
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Common/FindSignature.cpp
    src/formats/common/HandlerCont.cpp
    src/codecs/common/Rar5Decoder.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Crypto/Rar5Aes.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Crypto/HmacSha256.cpp)
set_target_properties(sevenzip_rar PROPERTIES AUTOMOC OFF AUTOUIC OFF)
target_include_directories(sevenzip_rar PUBLIC ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
target_include_directories(sevenzip_rar PRIVATE ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive)
set_source_files_properties(src/codecs/common/Rar5Decoder.cpp PROPERTIES
    INCLUDE_DIRECTORIES "${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress")
target_compile_definitions(sevenzip_rar PUBLIC Z7_EXTERNAL_CODECS)
target_link_libraries(sevenzip_rar PUBLIC sevenzip_zip)
target_link_libraries(sevenzip_common PUBLIC sevenzip_rar)

add_library(sevenzip_rar_codec_registry OBJECT
    src/codecs/common/RarCodecsRegister.cpp)
set_target_properties(sevenzip_rar_codec_registry PROPERTIES AUTOMOC OFF AUTOUIC OFF)
target_include_directories(sevenzip_rar_codec_registry PRIVATE ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
target_compile_definitions(sevenzip_rar_codec_registry PRIVATE Z7_EXTERNAL_CODECS)
target_link_libraries(sevenzip_common PUBLIC sevenzip_rar_codec_registry
    $<TARGET_OBJECTS:sevenzip_rar_codec_registry>)
