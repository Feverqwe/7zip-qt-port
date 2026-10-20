# Source ZIP handler and its codec/crypto/stream dependency closure.
# Baseline: docs/upstream-baseline.json.
add_library(sevenzip_zip STATIC
    src/archiveexternalcodecs.cpp
    third_party/7zip/C/7zStream.c
    third_party/7zip/C/Aes.c
    third_party/7zip/C/AesOpt.c
    third_party/7zip/C/Bra.c
    third_party/7zip/C/Bra86.c
    third_party/7zip/C/BraIA64.c
    third_party/7zip/C/Delta.c
    third_party/7zip/C/LzFind.c
    third_party/7zip/C/LzFindMt.c
    third_party/7zip/C/LzFindOpt.c
    third_party/7zip/C/Lzma2Dec.c
    third_party/7zip/C/Lzma2DecMt.c
    third_party/7zip/C/Lzma2Enc.c
    third_party/7zip/C/LzmaDec.c
    third_party/7zip/C/LzmaEnc.c
    third_party/7zip/C/MtCoder.c
    third_party/7zip/C/MtDec.c
    third_party/7zip/C/Ppmd8.c
    third_party/7zip/C/Ppmd8Dec.c
    third_party/7zip/C/Ppmd8Enc.c
    third_party/7zip/C/Xz.c
    third_party/7zip/C/XzDec.c
    third_party/7zip/C/XzEnc.c
    src/C/XzIn.c
    third_party/7zip/C/ZstdDec.c
    third_party/7zip/CPP/7zip/Archive/Common/HandlerOut.cpp
    third_party/7zip/CPP/7zip/Archive/Common/InStreamWithCRC.cpp
    third_party/7zip/CPP/7zip/Archive/Common/OutStreamWithCRC.cpp
    third_party/7zip/CPP/7zip/Archive/Common/ParseProperties.cpp
    third_party/7zip/CPP/7zip/Archive/Zip/ZipAddCommon.cpp
    third_party/7zip/CPP/7zip/Archive/Zip/ZipHandler.cpp
    third_party/7zip/CPP/7zip/Archive/Zip/ZipHandlerOut.cpp
    third_party/7zip/CPP/7zip/Archive/Zip/ZipIn.cpp
    third_party/7zip/CPP/7zip/Archive/Zip/ZipItem.cpp
    third_party/7zip/CPP/7zip/Archive/Zip/ZipOut.cpp
    third_party/7zip/CPP/7zip/Archive/Zip/ZipUpdate.cpp
    third_party/7zip/CPP/7zip/Common/CWrappers.cpp
    third_party/7zip/CPP/7zip/Common/InBuffer.cpp
    third_party/7zip/CPP/7zip/Common/MemBlocks.cpp
    third_party/7zip/CPP/7zip/Common/MethodId.cpp
    third_party/7zip/CPP/7zip/Common/OffsetStream.cpp
    third_party/7zip/CPP/7zip/Common/OutBuffer.cpp
    third_party/7zip/CPP/7zip/Common/OutMemStream.cpp
    third_party/7zip/CPP/7zip/Common/ProgressMt.cpp
    third_party/7zip/CPP/7zip/Common/ProgressUtils.cpp
    third_party/7zip/CPP/7zip/Compress/BitlDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/ImplodeDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/LzOutWindow.cpp
    third_party/7zip/CPP/7zip/Compress/LzfseDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/Lzma2Encoder.cpp
    src/CPP/7zip/Compress/LzmaDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/LzmaEncoder.cpp
    third_party/7zip/CPP/7zip/Compress/PpmdZip.cpp
    third_party/7zip/CPP/7zip/Compress/ShrinkDecoder.cpp
    src/CPP/7zip/Compress/XzDecoder.cpp
    third_party/7zip/CPP/7zip/Compress/XzEncoder.cpp
    src/CPP/7zip/Compress/ZstdDecoder.cpp
    third_party/7zip/CPP/7zip/Crypto/HmacSha1.cpp
    third_party/7zip/CPP/7zip/Crypto/MyAes.cpp
    third_party/7zip/CPP/7zip/Crypto/Pbkdf2HmacSha1.cpp
    third_party/7zip/CPP/7zip/Crypto/RandGen.cpp
    third_party/7zip/CPP/7zip/Crypto/WzAes.cpp
    third_party/7zip/CPP/7zip/Crypto/ZipCrypto.cpp
    third_party/7zip/CPP/7zip/Crypto/ZipStrong.cpp
    third_party/7zip/CPP/Common/MyVector.cpp
    third_party/7zip/CPP/Common/NewHandler.cpp
    third_party/7zip/CPP/Windows/PropVariantUtils.cpp
    third_party/7zip/CPP/Windows/Synchronization.cpp
    third_party/7zip/CPP/Windows/TimeUtils.cpp
)
set_target_properties(sevenzip_zip PROPERTIES AUTOMOC OFF AUTOUIC OFF)
target_include_directories(sevenzip_zip PUBLIC src third_party/7zip/CPP
    src/CPP/7zip/Archive/Common src/CPP/7zip/Archive/Zip)
set_source_files_properties(src/CPP/7zip/Compress/LzmaDecoder.cpp PROPERTIES
    INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/Compress")
set_source_files_properties(src/CPP/7zip/Compress/XzDecoder.cpp PROPERTIES
    INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/Compress")
set_source_files_properties(src/CPP/7zip/Compress/ZstdDecoder.cpp PROPERTIES
    INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/Compress")
set_source_files_properties(src/C/XzIn.c PROPERTIES
    INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/C")
target_link_libraries(sevenzip_zip PUBLIC sevenzip_common)
