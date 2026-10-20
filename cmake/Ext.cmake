# Linux ext4 C code is isolated in a GPL format module.
# Generic crypto retains C source boundaries and feeds the dynamic module's
# owned fscrypt inode/data/name readers.
add_library(sevenzip_fscrypt_crypto STATIC EXCLUDE_FROM_ALL
    src/lib/crypto/aes.c src/lib/crypto/utils.c src/lib/crypto/memneq.c
    src/lib/crypto/sha512.c src/fs/crypto/hkdf.c
    src/lib/crypto/sha256.c src/crypto/sm4.c src/lib/base64.c src/lib/siphash.c
    src/lib/crypto/gf128hash.c src/crypto/skciphercompat.c src/crypto/hctr2.c
    src/crypto/aesprovider.c src/crypto/essiv.c src/crypto/cts.c
    src/crypto/sm4_generic.c src/crypto/cbc.c src/crypto/ecb.c
    src/crypto/skcipherwalk.c src/crypto/xts.c src/crypto/chacha.c
    src/lib/crypto/chacha.c src/lib/crypto/chacha-block-generic.c
    src/lib/crypto/nh.c src/lib/crypto/poly1305-donna32.c
    src/crypto/adiantum.c src/crypto/skciphermemory.c
    src/fs/crypto/policy.c src/fs/crypto/keysetup.c src/fs/crypto/keysetup_v1.c
    src/fs/crypto/keyring.c src/fs/crypto/crypto.c src/fs/crypto/fname.c
    src/fs/crypto/hooks.c src/fscryptreader.c)
target_include_directories(sevenzip_fscrypt_crypto PUBLIC src/include src)
# Linux's generic word-at-a-time crypto routines use the kernel's aliasing
# contract, including byte arrays accessed through wider integer pointers.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(sevenzip_fscrypt_crypto PRIVATE -fno-strict-aliasing)
endif()
set_target_properties(sevenzip_fscrypt_crypto PROPERTIES AUTOMOC OFF AUTOUIC OFF
    C_STANDARD 11 C_STANDARD_REQUIRED ON POSITION_INDEPENDENT_CODE ON)
if(BUILD_TESTING)
    add_executable(fscrypt_crypto_tests tests/fscrypt_crypto_tests.c tests/fscrypt_support_tests.c
        tests/fscrypt_modes_tests.c tests/fscrypt_policy_tests.c tests/fscrypt_reader_tests.c)
    target_link_libraries(fscrypt_crypto_tests PRIVATE sevenzip_fscrypt_crypto)
    set_target_properties(fscrypt_crypto_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF
        C_STANDARD 11 C_STANDARD_REQUIRED ON)
    add_test(NAME fscrypt_crypto COMMAND fscrypt_crypto_tests)
    set_tests_properties(fscrypt_crypto PROPERTIES TIMEOUT 30)
endif()
add_library(sevenzip_ext MODULE
    src/fs/ext4/acl.c src/fs/posix_acl.c
    src/CPP/7zip/Archive/ExtHandler.cpp src/exfatmodule.cpp
    src/extreader.c src/fs/ext4/super.c src/fs/ext4/crypto.c src/fs/ext4/balloc.c
    src/fs/ext4/inode.c src/fs/ext4/bitmap.c src/fs/ext4/inline.c
    src/fs/ext4/xattr.c src/fs/ext4/dir.c src/fs/ext4/namei.c src/fs/ext4/hash.c
    src/fs/ext4/extents.c src/lib/crc/crc16.c src/lib/crc/crc32-main.c
    src/fs/ext4/verity.c src/fs/verity/open.c src/fs/verity/hash_algs.c
    src/fs/verity/verify.c src/fs/verity/pagecache.c
    src/extverityreader.c src/extveritystream.cpp src/extfscrypt.cpp
    third_party/7zip/CPP/7zip/Archive/Common/ItemNameUtils.cpp
    third_party/7zip/CPP/7zip/Archive/ArchiveExports.cpp
    third_party/7zip/CPP/7zip/Common/StreamObjects.cpp
    third_party/7zip/CPP/7zip/Common/StreamUtils.cpp
    src/CPP/7zip/Common/LimitedStreams.cpp
    third_party/7zip/CPP/7zip/Common/ProgressUtils.cpp
    third_party/7zip/CPP/7zip/Common/PropId.cpp
    third_party/7zip/CPP/7zip/Compress/CopyCoder.cpp
    third_party/7zip/CPP/Windows/PropVariant.cpp
    third_party/7zip/CPP/Windows/PropVariantUtils.cpp
    third_party/7zip/CPP/Windows/PropVariantConv.cpp
    third_party/7zip/CPP/Windows/TimeUtils.cpp
    third_party/7zip/CPP/Common/MyWindows.cpp
    third_party/7zip/CPP/Common/MyString.cpp
    third_party/7zip/CPP/Common/MyVector.cpp
    third_party/7zip/CPP/Common/IntToString.cpp
    third_party/7zip/CPP/Common/StringToInt.cpp
    third_party/7zip/CPP/Common/StringConvert.cpp
    third_party/7zip/CPP/Common/UTFConvert.cpp
    third_party/7zip/C/CpuArch.c third_party/7zip/C/Alloc.c
    third_party/7zip/C/Sha256.c third_party/7zip/C/Sha256Opt.c
    third_party/7zip/C/Sha512.c third_party/7zip/C/Sha512Opt.c)
target_include_directories(sevenzip_ext PRIVATE src third_party/7zip/CPP
    third_party/7zip/CPP/7zip/Archive)
target_compile_definitions(sevenzip_ext PRIVATE SEVENZIP_LINUX_EXT)
target_link_libraries(sevenzip_ext PRIVATE Threads::Threads sevenzip_fscrypt_crypto)
set_target_properties(sevenzip_ext PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_ext PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_ext PRIVATE src/exfatmodule.def)
    target_link_libraries(sevenzip_ext PRIVATE oleaut32)
endif()
function(sevenzip_deploy_ext)
    add_custom_target(sevenzip_ext_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_ext>"
            "${CMAKE_CURRENT_SOURCE_DIR}/build/codec-runtime/Formats/$<TARGET_FILE_NAME:sevenzip_ext>"
        DEPENDS sevenzip_ext VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_ext_runtime)
endfunction()
