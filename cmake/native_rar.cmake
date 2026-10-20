# Original RAR5/RAR7 decoder and handler support. The handler itself remains
# the single original Rar5Handler.cpp in the forced hash/arc registration object.
# Baseline: docs/upstream-baseline.json.
add_library(sevenzip_rar STATIC
    third_party/7zip/CPP/7zip/Archive/Common/FindSignature.cpp
    src/CPP/7zip/Archive/HandlerCont.cpp
    third_party/7zip/CPP/7zip/Compress/Rar5Decoder.cpp
    third_party/7zip/CPP/7zip/Crypto/Rar5Aes.cpp
    third_party/7zip/CPP/7zip/Crypto/HmacSha256.cpp)
set_target_properties(sevenzip_rar PROPERTIES AUTOMOC OFF AUTOUIC OFF)
target_include_directories(sevenzip_rar PUBLIC src/CPP src third_party/7zip/CPP)
target_include_directories(sevenzip_rar PRIVATE third_party/7zip/CPP/7zip/Archive)
target_compile_definitions(sevenzip_rar PUBLIC Z7_EXTERNAL_CODECS)
target_link_libraries(sevenzip_rar PUBLIC sevenzip_zip)
target_link_libraries(sevenzip_common PUBLIC sevenzip_rar)

add_library(sevenzip_rar_codec_registry OBJECT
    src/CPP/7zip/Compress/RarCodecsRegister.cpp)
set_target_properties(sevenzip_rar_codec_registry PROPERTIES AUTOMOC OFF AUTOUIC OFF)
target_include_directories(sevenzip_rar_codec_registry PRIVATE src/CPP src third_party/7zip/CPP)
target_compile_definitions(sevenzip_rar_codec_registry PRIVATE Z7_EXTERNAL_CODECS)
target_link_libraries(sevenzip_common PUBLIC sevenzip_rar_codec_registry
    $<TARGET_OBJECTS:sevenzip_rar_codec_registry>)
