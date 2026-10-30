# Source ZIP handler and its codec/crypto/stream dependency closure.
# Baseline: docs/upstream-baseline.json.
add_library(sevenzip_zip STATIC
    src/archive/codecs/archiveexternalcodecs.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/7zStream.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Aes.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/AesOpt.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Bra.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Bra86.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/BraIA64.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Delta.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/LzFind.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/LzFindMt.c
    src/codecs/lzma/LzFindOpt.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Lzma2Dec.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Lzma2DecMt.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Lzma2Enc.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/LzmaDec.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/LzmaEnc.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/MtCoder.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/MtDec.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Ppmd8.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Ppmd8Dec.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Ppmd8Enc.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Xz.c
    src/codecs/xz/XzDec.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/XzEnc.c
    src/codecs/xz/XzIn.c
    src/codecs/zstd/ZstdDec.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Common/HandlerOut.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Common/InStreamWithCRC.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Common/OutStreamWithCRC.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Common/ParseProperties.cpp
    src/formats/zip/ZipAddCommon.cpp
    src/formats/zip/ZipHandler.cpp
    src/formats/zip/ZipHandlerOut.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Zip/ZipIn.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Zip/ZipItem.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Zip/ZipOut.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Zip/ZipUpdate.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/CWrappers.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/InBuffer.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/MemBlocks.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/MethodId.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/OffsetStream.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/OutBuffer.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/OutMemStream.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/ProgressMt.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/ProgressUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/BitlDecoder.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/ImplodeDecoder.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/LzOutWindow.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/LzfseDecoder.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/Lzma2Encoder.cpp
    src/codecs/lzma/LzmaDecoder.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/LzmaEncoder.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/PpmdZip.cpp
    src/codecs/shrink/ShrinkDecoder.cpp
    src/codecs/xz/XzDecoder.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/XzEncoder.cpp
    src/codecs/zstd/ZstdDecoder.cpp
    src/codecs/zstd/ZstdEncoder.cpp
    src/crypto/hmac/HmacSha1.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Crypto/MyAes.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Crypto/Pbkdf2HmacSha1.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Crypto/RandGen.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Crypto/WzAes.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Crypto/ZipCrypto.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Crypto/ZipStrong.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyVector.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/NewHandler.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/PropVariantUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/Synchronization.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/TimeUtils.cpp
)
set_target_properties(sevenzip_zip PROPERTIES AUTOMOC OFF AUTOUIC OFF)
set_source_files_properties(src/crypto/hmac/HmacSha1.cpp PROPERTIES
    INCLUDE_DIRECTORIES "${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Crypto")
target_include_directories(sevenzip_zip PUBLIC src ${SEVENZIP_NATIVE_INCLUDE_DIR}/C ${SEVENZIP_7ZIP_SOURCE_DIR}/C ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP
    ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP/7zip/Archive/Common ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP/7zip/Archive/Zip
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Zip ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress)
set_source_files_properties(src/codecs/lzma/LzmaDecoder.cpp PROPERTIES
    INCLUDE_DIRECTORIES "${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress")
set_source_files_properties(src/codecs/xz/XzDecoder.cpp PROPERTIES
    INCLUDE_DIRECTORIES "${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress")
set_source_files_properties(src/codecs/zstd/ZstdDecoder.cpp PROPERTIES
    INCLUDE_DIRECTORIES "${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress")
set_source_files_properties(src/codecs/xz/XzIn.c src/codecs/xz/XzDec.c PROPERTIES
    INCLUDE_DIRECTORIES "${SEVENZIP_7ZIP_SOURCE_DIR}/C")
target_link_libraries(sevenzip_zip PUBLIC sevenzip_common sevenzip_zstd)

set_source_files_properties(src/codecs/zstd/ZstdEncoder.cpp PROPERTIES
    INCLUDE_DIRECTORIES "${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress")
set_source_files_properties(src/formats/zip/ZipAddCommon.cpp PROPERTIES
    INCLUDE_DIRECTORIES "${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Zip")

if(BUILD_TESTING)
    foreach(primitive hash aes pbkdf2)
        add_executable(crypto_${primitive}_tests tests/crypto/crypto_${primitive}_tests.cpp)
        set_target_properties(crypto_${primitive}_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF)
        target_link_libraries(crypto_${primitive}_tests PRIVATE sevenzip_zip)
        add_test(NAME crypto_${primitive} COMMAND crypto_${primitive}_tests)
        set_tests_properties(crypto_${primitive} PROPERTIES TIMEOUT 15)
    endforeach()
    qt_add_executable(crypto_zip_tests tests/formats/zip/crypto_zip_tests.cpp)
    set_target_properties(crypto_zip_tests PROPERTIES AUTOMOC ON AUTOUIC OFF)
    target_include_directories(crypto_zip_tests PRIVATE tests)
    target_link_libraries(crypto_zip_tests PRIVATE sevenzip_zip Qt6::Test)
    target_compile_definitions(crypto_zip_tests PRIVATE
        "SEVENZIP_CRYPTO_REFERENCE_7Z=\"${SEVENZIP_CODEC_RUNTIME_DIR}/7z${CMAKE_EXECUTABLE_SUFFIX}\"")
    add_test(NAME crypto_zip COMMAND crypto_zip_tests -silent)
    set_tests_properties(crypto_zip PROPERTIES TIMEOUT 45)
    qt_add_executable(shared_checksum_tests tests/archive/shared_checksum_tests.cpp src/compat/linux/core/xxhash.c
        src/codecs/zlib/ZlibDecoder.cpp
        ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/DeflateDecoder.cpp
        ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/BitlDecoder.cpp
        ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/LzOutWindow.cpp)
    target_include_directories(shared_checksum_tests PRIVATE third_party)
    target_link_libraries(shared_checksum_tests PRIVATE sevenzip_zip sevenzip_zisofs_zlib Qt6::Test)
    add_test(NAME shared_checksum COMMAND shared_checksum_tests -silent)
    set_tests_properties(shared_checksum PROPERTIES TIMEOUT 30)
endif()
