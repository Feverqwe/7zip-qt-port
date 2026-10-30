# Linux ext4 C code is isolated in a GPL format module.
# Generic crypto retains C source boundaries and feeds the dynamic module's
# owned fscrypt inode/data/name readers.
add_executable(linux_asn1_compiler EXCLUDE_FROM_ALL tools/asn1/asn1_compiler.c)
target_include_directories(linux_asn1_compiler PRIVATE src/compat/linux/include)
if(MSVC)
    target_compile_definitions(linux_asn1_compiler PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
set_target_properties(linux_asn1_compiler PROPERTIES AUTOMOC OFF AUTOUIC OFF
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
add_library(sevenzip_fscrypt_crypto STATIC EXCLUDE_FROM_ALL
    src/crypto/primitives/sdkaesblocks.c src/crypto/primitives/sdkaesinit.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Aes.c ${SEVENZIP_7ZIP_SOURCE_DIR}/C/AesOpt.c
    src/crypto/primitives/sdkhashblocks.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Sha256.c ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Sha512.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Sha256Opt.c ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Sha512Opt.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Sha3.c
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/CpuArch.c
    src/compat/linux/core/asn1_decoder.c src/compat/linux/core/oid_registry.c
    src/crypto/certificates/asymmetric_type.c
    src/crypto/certificates/public_key.c src/crypto/certificates/signature.c
    src/crypto/certificates/x509_cert_parser.c src/crypto/certificates/pkcs7_parser.c
    src/crypto/certificates/x509_public_key.c
    src/crypto/certificates/pkcs7_verify.c
    src/crypto/certificates/pkcs7_trust.c
    src/crypto/certificates/keyringprovider.c src/crypto/certificates/system_keyring.c
    src/compat/linux/core/hexdump.c
    src/crypto/primitives/hash_info.c src/compat/linux/core/string_helpers.c
    src/compat/linux/time/time.c
    src/crypto/primitives/aes.c src/crypto/primitives/utils.c src/crypto/primitives/memneq.c
    src/crypto/primitives/sha512.c src/crypto/primitives/sha1.c src/crypto/primitives/sha3.c
    src/crypto/modes/shashprovider.c src/crypto/modes/shash.c src/crypto/modes/streebog_generic.c
    src/crypto/primitives/sm3.c src/crypto/primitives/mldsa.c src/compat/linux/core/string.c
    src/crypto/fscrypt/hkdf.c
    src/crypto/primitives/sha256.c src/crypto/modes/sm4.c src/compat/linux/core/base64.c src/compat/linux/core/siphash.c
    src/crypto/primitives/gf128hash.c src/crypto/modes/skciphercompat.c src/crypto/modes/hctr2.c
    src/crypto/modes/aesprovider.c src/crypto/modes/essiv.c src/crypto/modes/cts.c
    src/crypto/modes/sm4_generic.c src/crypto/modes/cbc.c src/crypto/modes/ecb.c
    src/crypto/modes/skcipherwalk.c src/crypto/modes/xts.c src/crypto/modes/chacha.c
    src/crypto/primitives/chacha.c src/crypto/primitives/chacha-block-generic.c
    src/crypto/primitives/nh.c src/crypto/primitives/poly1305-donna32.c
    src/crypto/modes/adiantum.c src/crypto/modes/skciphermemory.c
    src/crypto/fscrypt/policy.c src/crypto/fscrypt/keysetup.c src/crypto/fscrypt/keysetup_v1.c
    src/crypto/fscrypt/keyring.c src/crypto/fscrypt/crypto.c src/crypto/fscrypt/fname.c
    src/crypto/fscrypt/hooks.c src/crypto/fscrypt/fscryptreader.c)
target_sources(sevenzip_fscrypt_crypto PRIVATE
    src/crypto/modes/rsa.c src/crypto/modes/rsa_helper.c src/crypto/modes/rsassa-pkcs1.c
    src/crypto/modes/akcipher.c src/crypto/modes/akcipherprovider.c
    src/crypto/modes/sig.c src/crypto/modes/sigprovider.c src/crypto/modes/mldsa.c
    src/crypto/modes/ecc.c src/crypto/modes/ecdsa.c src/crypto/modes/ecrdsa.c
    src/crypto/modes/ecdsa-x962.c src/crypto/modes/ecdsa-p1363.c
    src/compat/linux/core/scatterlist.c
    src/crypto/primitives/mpi/generic_mpih-add1.c
    src/crypto/primitives/mpi/generic_mpih-lshift.c
    src/crypto/primitives/mpi/generic_mpih-mul1.c
    src/crypto/primitives/mpi/generic_mpih-mul2.c
    src/crypto/primitives/mpi/generic_mpih-mul3.c
    src/crypto/primitives/mpi/generic_mpih-rshift.c
    src/crypto/primitives/mpi/generic_mpih-sub1.c
    src/crypto/primitives/mpi/mpi-add.c src/crypto/primitives/mpi/mpi-bit.c
    src/crypto/primitives/mpi/mpi-cmp.c src/crypto/primitives/mpi/mpi-div.c
    src/crypto/primitives/mpi/mpi-mod.c src/crypto/primitives/mpi/mpi-mul.c
    src/crypto/primitives/mpi/mpi-pow.c src/crypto/primitives/mpi/mpi-sub-ui.c
    src/crypto/primitives/mpi/mpicoder.c src/crypto/primitives/mpi/mpih-cmp.c
    src/crypto/primitives/mpi/mpih-div.c src/crypto/primitives/mpi/mpih-mul.c
    src/crypto/primitives/mpi/mpiutil.c)
target_include_directories(sevenzip_fscrypt_crypto PUBLIC src/compat/linux/include src)
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
    set(asymmetric_input "src/crypto/certificates/${asymmetric_grammar}.asn1")
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
    set(rsa_input "src/crypto/modes/${rsa_grammar}.asn1")
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
    add_executable(asymmetric_signature_tests tests/crypto/asymmetric_signature_tests.c
        tests/crypto/asymmetric_rsa_tests.c tests/crypto/asymmetric_ecc_tests.c)
    target_link_libraries(asymmetric_signature_tests PRIVATE sevenzip_fscrypt_crypto)
    set_target_properties(asymmetric_signature_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF
        C_STANDARD 11 C_STANDARD_REQUIRED ON)
    add_test(NAME asymmetric_signature COMMAND asymmetric_signature_tests)
    set_tests_properties(asymmetric_signature PROPERTIES TIMEOUT 15)
    add_executable(asymmetric_certificate_tests tests/crypto/asymmetric_certificate_tests.c)
    target_link_libraries(asymmetric_certificate_tests PRIVATE sevenzip_fscrypt_crypto)
    set_target_properties(asymmetric_certificate_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF
        C_STANDARD 11 C_STANDARD_REQUIRED ON)
    add_test(NAME asymmetric_certificate COMMAND asymmetric_certificate_tests)
    set_tests_properties(asymmetric_certificate PROPERTIES TIMEOUT 15)
    add_executable(fsverity_signature_tests tests/crypto/fsverity_signature_tests.c
        src/crypto/verity/signature.c src/crypto/verity/hash_algs.c)
    target_link_libraries(fsverity_signature_tests PRIVATE sevenzip_fscrypt_crypto)
    set_target_properties(fsverity_signature_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF
        C_STANDARD 11 C_STANDARD_REQUIRED ON)
    add_test(NAME fsverity_signature COMMAND fsverity_signature_tests)
    set_tests_properties(fsverity_signature PROPERTIES TIMEOUT 15)
    add_executable(asymmetric_hash_tests tests/crypto/asymmetric_hash_tests.c)
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
    add_executable(asn1_reader_tests tests/crypto/asn1_reader_tests.c "${asn1_test_dir}/spki.asn1.c")
    target_include_directories(asn1_reader_tests PRIVATE "${asn1_test_dir}")
    target_link_libraries(asn1_reader_tests PRIVATE sevenzip_fscrypt_crypto)
    set_target_properties(asn1_reader_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF
        C_STANDARD 11 C_STANDARD_REQUIRED ON)
    add_test(NAME asn1_reader COMMAND asn1_reader_tests)
    set_tests_properties(asn1_reader PROPERTIES TIMEOUT 15)
    add_executable(fscrypt_crypto_tests tests/crypto/fscrypt_crypto_tests.c tests/crypto/support/fscrypt_support_tests.c
        tests/crypto/fscrypt_modes_tests.c tests/crypto/fscrypt_policy_tests.c tests/crypto/fscrypt_reader_tests.c)
    target_link_libraries(fscrypt_crypto_tests PRIVATE sevenzip_fscrypt_crypto)
    set_target_properties(fscrypt_crypto_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF
        C_STANDARD 11 C_STANDARD_REQUIRED ON)
    add_test(NAME fscrypt_crypto COMMAND fscrypt_crypto_tests)
    set_tests_properties(fscrypt_crypto PROPERTIES TIMEOUT 30)
endif()
add_library(sevenzip_ext MODULE
    src/formats/ext/ExtHeader.cpp
    src/formats/ext/acl.c src/compat/linux/filesystem/posix_acl.c
    src/formats/ext/ExtHandler.cpp src/formats/common/moduleexports.cpp src/formats/ext/extpasswordguid.cpp
    src/formats/ext/extreader.c src/compat/linux/filesystem/readdir.c src/compat/linux/filesystem/libfs.c src/formats/ext/super.c src/formats/ext/crypto.c src/formats/ext/balloc.c
    src/formats/ext/inode.c src/formats/ext/bitmap.c src/formats/ext/inline.c src/formats/ext/symlink.c
    src/formats/ext/indirect.c src/formats/ext/extindirectstream.cpp
    src/formats/ext/block_validity.c src/compat/linux/core/rbtree.c src/compat/linux/core/find_bit.c src/formats/ext/extblockreader.c
    src/formats/ext/xattr.c src/formats/ext/dir.c src/formats/ext/namei.c src/formats/ext/hash.c
    src/formats/ext/extents.c src/checksums/linux/crc16.c src/checksums/linux/crc32-main.c
    src/formats/ext/verity.c src/crypto/verity/open.c src/crypto/verity/hash_algs.c
    src/crypto/verity/verify.c src/crypto/verity/pagecache.c src/crypto/verity/signature.c
    src/formats/ext/extverityreader.c src/formats/ext/extveritystream.cpp src/formats/ext/extveritypolicy.cpp src/formats/ext/extfscrypt.cpp
    src/archive/compression/MethodProps.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/Common/ItemNameUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive/ArchiveExports.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/StreamObjects.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/StreamUtils.cpp
    src/archive/streams/LimitedStreams.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/ProgressUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Common/PropId.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Compress/CopyCoder.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/PropVariant.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/PropVariantUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/PropVariantConv.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Windows/TimeUtils.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyWindows.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyString.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/MyVector.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/IntToString.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/StringToInt.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/StringConvert.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/Common/UTFConvert.cpp
    ${SEVENZIP_7ZIP_SOURCE_DIR}/C/Alloc.c)
target_include_directories(sevenzip_ext PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP
    ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP/7zip/Archive)
target_link_libraries(sevenzip_ext PRIVATE Threads::Threads sevenzip_fscrypt_crypto
    sevenzip_liblzma_crc sevenzip_crc32c)
set_target_properties(sevenzip_ext PROPERTIES AUTOMOC OFF AUTOUIC OFF PREFIX ""
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_options(sevenzip_ext PRIVATE "-Wl,-Bsymbolic")
endif()
if(WIN32)
    target_sources(sevenzip_ext PRIVATE src/formats/common/moduleexports.def)
    target_link_libraries(sevenzip_ext PRIVATE oleaut32)
endif()
function(sevenzip_deploy_ext)
    add_custom_target(sevenzip_ext_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:sevenzip_ext>"
            "${SEVENZIP_CODEC_RUNTIME_DIR}/Formats/$<TARGET_FILE_NAME:sevenzip_ext>"
        DEPENDS sevenzip_ext VERBATIM)
    add_dependencies(sevenzip_desktop sevenzip_ext_runtime)
endfunction()

# The GPL reader sources use their own upstream-shaped include root.
target_include_directories(sevenzip_ext BEFORE PRIVATE ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP ${SEVENZIP_NATIVE_INCLUDE_DIR}/CPP ${SEVENZIP_7ZIP_SOURCE_DIR})
