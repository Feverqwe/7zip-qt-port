# Linux ext4 C code is isolated in a GPL format module.
# Generic crypto retains C source boundaries and feeds the dynamic module's
# owned fscrypt inode/data/name readers.
add_executable(linux_asn1_compiler EXCLUDE_FROM_ALL src/scripts/asn1_compiler.c)
target_include_directories(linux_asn1_compiler PRIVATE src/include)
if(MSVC)
    target_compile_definitions(linux_asn1_compiler PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
set_target_properties(linux_asn1_compiler PROPERTIES AUTOMOC OFF AUTOUIC OFF
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
add_library(sevenzip_fscrypt_crypto STATIC EXCLUDE_FROM_ALL
    src/lib/asn1_decoder.c src/lib/oid_registry.c
    src/crypto/asymmetric_keys/asymmetric_type.c
    src/crypto/asymmetric_keys/public_key.c src/crypto/asymmetric_keys/signature.c
    src/crypto/asymmetric_keys/x509_cert_parser.c src/crypto/asymmetric_keys/pkcs7_parser.c
    src/crypto/asymmetric_keys/x509_public_key.c
    src/crypto/asymmetric_keys/pkcs7_verify.c
    src/crypto/asymmetric_keys/pkcs7_trust.c
    src/crypto/asymmetric_keys/keyringprovider.c src/certs/system_keyring.c
    src/lib/hexdump.c
    src/lib/crypto/hash_info.c src/lib/string_helpers.c
    src/kernel/time/time.c
    src/lib/crypto/aes.c src/lib/crypto/utils.c src/lib/crypto/memneq.c
    src/lib/crypto/sha512.c src/lib/crypto/sha1.c src/lib/crypto/sha3.c
    src/crypto/shashprovider.c src/crypto/shash.c src/crypto/streebog_generic.c
    src/lib/crypto/sm3.c src/lib/crypto/mldsa.c src/lib/string.c
    src/fs/crypto/hkdf.c
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
target_sources(sevenzip_fscrypt_crypto PRIVATE
    src/crypto/rsa.c src/crypto/rsa_helper.c src/crypto/rsassa-pkcs1.c
    src/crypto/akcipher.c src/crypto/akcipherprovider.c
    src/crypto/sig.c src/crypto/sigprovider.c src/crypto/mldsa.c
    src/crypto/ecc.c src/crypto/ecdsa.c src/crypto/ecrdsa.c
    src/crypto/ecdsa-x962.c src/crypto/ecdsa-p1363.c
    src/lib/scatterlist.c
    src/lib/crypto/mpi/generic_mpih-add1.c
    src/lib/crypto/mpi/generic_mpih-lshift.c
    src/lib/crypto/mpi/generic_mpih-mul1.c
    src/lib/crypto/mpi/generic_mpih-mul2.c
    src/lib/crypto/mpi/generic_mpih-mul3.c
    src/lib/crypto/mpi/generic_mpih-rshift.c
    src/lib/crypto/mpi/generic_mpih-sub1.c
    src/lib/crypto/mpi/mpi-add.c src/lib/crypto/mpi/mpi-bit.c
    src/lib/crypto/mpi/mpi-cmp.c src/lib/crypto/mpi/mpi-div.c
    src/lib/crypto/mpi/mpi-mod.c src/lib/crypto/mpi/mpi-mul.c
    src/lib/crypto/mpi/mpi-pow.c src/lib/crypto/mpi/mpi-sub-ui.c
    src/lib/crypto/mpi/mpicoder.c src/lib/crypto/mpi/mpih-cmp.c
    src/lib/crypto/mpi/mpih-div.c src/lib/crypto/mpi/mpih-mul.c
    src/lib/crypto/mpi/mpiutil.c)
target_include_directories(sevenzip_fscrypt_crypto PUBLIC src/include src)
target_compile_definitions(sevenzip_fscrypt_crypto PRIVATE CONFIG_SYSTEM_DATA_VERIFICATION)
# Linux's generic word-at-a-time crypto routines use the kernel's aliasing
# contract, including byte arrays accessed through wider integer pointers.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(sevenzip_fscrypt_crypto PRIVATE -fno-strict-aliasing)
endif()
set_target_properties(sevenzip_fscrypt_crypto PROPERTIES AUTOMOC OFF AUTOUIC OFF
    C_STANDARD 11 C_STANDARD_REQUIRED ON POSITION_INDEPENDENT_CODE ON)
set(asymmetric_asn1_dir "${CMAKE_CURRENT_BINARY_DIR}/asymmetric-asn1")
target_include_directories(sevenzip_fscrypt_crypto PRIVATE "${asymmetric_asn1_dir}")
foreach(asymmetric_grammar IN ITEMS x509 x509_akid pkcs7)
    set(asymmetric_input "src/crypto/asymmetric_keys/${asymmetric_grammar}.asn1")
    set(asymmetric_c "${asymmetric_asn1_dir}/${asymmetric_grammar}.asn1.c")
    set(asymmetric_h "${asymmetric_asn1_dir}/${asymmetric_grammar}.asn1.h")
    add_custom_command(OUTPUT "${asymmetric_c}" "${asymmetric_h}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${asymmetric_asn1_dir}"
        COMMAND $<TARGET_FILE:linux_asn1_compiler>
            "${CMAKE_CURRENT_SOURCE_DIR}/${asymmetric_input}" "${asymmetric_c}" "${asymmetric_h}"
        DEPENDS linux_asn1_compiler "${asymmetric_input}" VERBATIM)
    target_sources(sevenzip_fscrypt_crypto PRIVATE "${asymmetric_c}")
endforeach()
foreach(rsa_grammar IN ITEMS rsapubkey rsaprivkey ecdsasignature ecrdsa_params ecrdsa_pub_key)
    set(rsa_input "src/crypto/${rsa_grammar}.asn1")
    set(rsa_c "${asymmetric_asn1_dir}/${rsa_grammar}.asn1.c")
    set(rsa_h "${asymmetric_asn1_dir}/${rsa_grammar}.asn1.h")
    add_custom_command(OUTPUT "${rsa_c}" "${rsa_h}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${asymmetric_asn1_dir}"
        COMMAND $<TARGET_FILE:linux_asn1_compiler>
            "${CMAKE_CURRENT_SOURCE_DIR}/${rsa_input}" "${rsa_c}" "${rsa_h}"
        DEPENDS linux_asn1_compiler "${rsa_input}" VERBATIM)
    target_sources(sevenzip_fscrypt_crypto PRIVATE "${rsa_c}")
endforeach()
if(BUILD_TESTING)
    add_executable(asymmetric_signature_tests tests/asymmetric_signature_tests.c
        tests/asymmetric_rsa_tests.c tests/asymmetric_ecc_tests.c)
    target_link_libraries(asymmetric_signature_tests PRIVATE sevenzip_fscrypt_crypto)
    set_target_properties(asymmetric_signature_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF
        C_STANDARD 11 C_STANDARD_REQUIRED ON)
    add_test(NAME asymmetric_signature COMMAND asymmetric_signature_tests)
    set_tests_properties(asymmetric_signature PROPERTIES TIMEOUT 15)
    add_executable(asymmetric_certificate_tests tests/asymmetric_certificate_tests.c)
    target_link_libraries(asymmetric_certificate_tests PRIVATE sevenzip_fscrypt_crypto)
    set_target_properties(asymmetric_certificate_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF
        C_STANDARD 11 C_STANDARD_REQUIRED ON)
    add_test(NAME asymmetric_certificate COMMAND asymmetric_certificate_tests)
    set_tests_properties(asymmetric_certificate PROPERTIES TIMEOUT 15)
    add_executable(fsverity_signature_tests tests/fsverity_signature_tests.c
        src/fs/verity/signature.c src/fs/verity/hash_algs.c
        third_party/7zip/C/CpuArch.c src/C/Sha256.c
        third_party/7zip/C/Sha256Opt.c src/C/Sha512.c
        third_party/7zip/C/Sha512Opt.c)
    target_link_libraries(fsverity_signature_tests PRIVATE sevenzip_fscrypt_crypto)
    set_target_properties(fsverity_signature_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF
        C_STANDARD 11 C_STANDARD_REQUIRED ON)
    add_test(NAME fsverity_signature COMMAND fsverity_signature_tests)
    set_tests_properties(fsverity_signature PROPERTIES TIMEOUT 15)
    add_executable(asymmetric_hash_tests tests/asymmetric_hash_tests.c)
    target_link_libraries(asymmetric_hash_tests PRIVATE sevenzip_fscrypt_crypto)
    set_target_properties(asymmetric_hash_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF
        C_STANDARD 11 C_STANDARD_REQUIRED ON)
    add_test(NAME asymmetric_hash COMMAND asymmetric_hash_tests)
    set_tests_properties(asymmetric_hash PROPERTIES TIMEOUT 15)
    set(asn1_test_dir "${CMAKE_CURRENT_BINARY_DIR}/asn1")
    add_custom_command(OUTPUT "${asn1_test_dir}/spki.asn1.c" "${asn1_test_dir}/spki.asn1.h"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${asn1_test_dir}"
        COMMAND $<TARGET_FILE:linux_asn1_compiler>
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/asn1/spki.asn1"
            "${asn1_test_dir}/spki.asn1.c" "${asn1_test_dir}/spki.asn1.h"
        DEPENDS linux_asn1_compiler tests/fixtures/asn1/spki.asn1 VERBATIM)
    add_executable(asn1_reader_tests tests/asn1_reader_tests.c "${asn1_test_dir}/spki.asn1.c")
    target_include_directories(asn1_reader_tests PRIVATE "${asn1_test_dir}")
    target_link_libraries(asn1_reader_tests PRIVATE sevenzip_fscrypt_crypto)
    set_target_properties(asn1_reader_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF
        C_STANDARD 11 C_STANDARD_REQUIRED ON)
    add_test(NAME asn1_reader COMMAND asn1_reader_tests)
    set_tests_properties(asn1_reader PROPERTIES TIMEOUT 15)
    add_executable(fscrypt_crypto_tests tests/fscrypt_crypto_tests.c tests/fscrypt_support_tests.c
        tests/fscrypt_modes_tests.c tests/fscrypt_policy_tests.c tests/fscrypt_reader_tests.c)
    target_link_libraries(fscrypt_crypto_tests PRIVATE sevenzip_fscrypt_crypto)
    set_target_properties(fscrypt_crypto_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF
        C_STANDARD 11 C_STANDARD_REQUIRED ON)
    add_test(NAME fscrypt_crypto COMMAND fscrypt_crypto_tests)
    set_tests_properties(fscrypt_crypto PROPERTIES TIMEOUT 30)
endif()
add_library(sevenzip_ext MODULE
    src/CPP/7zip/Archive/ExtHeader.cpp
    src/fs/ext4/acl.c src/fs/posix_acl.c
    src/gpl/CPP/7zip/Archive/ExtHandler.cpp src/exfatmodule.cpp src/extpasswordguid.cpp
    src/extreader.c src/fs/readdir.c src/fs/libfs.c src/fs/ext4/super.c src/fs/ext4/crypto.c src/fs/ext4/balloc.c
    src/fs/ext4/inode.c src/fs/ext4/bitmap.c src/fs/ext4/inline.c src/fs/ext4/symlink.c
    src/fs/ext4/indirect.c src/extindirectstream.cpp
    src/fs/ext4/block_validity.c src/lib/rbtree.c src/lib/find_bit.c src/extblockreader.c
    src/fs/ext4/xattr.c src/fs/ext4/dir.c src/fs/ext4/namei.c src/fs/ext4/hash.c
    src/fs/ext4/extents.c src/lib/crc/crc16.c src/lib/crc/crc32-main.c
    src/fs/ext4/verity.c src/fs/verity/open.c src/fs/verity/hash_algs.c
    src/fs/verity/verify.c src/fs/verity/pagecache.c src/fs/verity/signature.c
    src/extverityreader.c src/extveritystream.cpp src/extveritypolicy.cpp src/extfscrypt.cpp
    src/CPP/7zip/Common/MethodProps.cpp
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
    src/C/Sha256.c third_party/7zip/C/Sha256Opt.c
    src/C/Sha512.c third_party/7zip/C/Sha512Opt.c)
target_include_directories(sevenzip_ext PRIVATE src third_party/7zip/CPP
    third_party/7zip/CPP/7zip/Archive)
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

# The GPL reader sources use their own upstream-shaped include root.
target_include_directories(sevenzip_ext BEFORE PRIVATE src/gpl/CPP src/CPP third_party/7zip)
