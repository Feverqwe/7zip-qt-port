# SDK/application crypto requires libcrypto. GPL filesystem targets keep their
# independent implementations and never link this target.
unset(SEVENZIP_CRYPTO_PROVIDER CACHE)
if(NOT DEFINED OPENSSL_USE_STATIC_LIBS)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()
find_package(OpenSSL 3 REQUIRED COMPONENTS Crypto)
include(CheckCSourceCompiles)
include(CMakePushCheckState)
file(GLOB crypto_headers CONFIGURE_DEPENDS "${OPENSSL_INCLUDE_DIR}/openssl/*.h")
set(crypto_header_inputs "")
foreach(header IN LISTS crypto_headers)
    file(SHA256 "${header}" digest)
    string(APPEND crypto_header_inputs "${header}:${digest};")
endforeach()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${OPENSSL_CRYPTO_LIBRARY}" ${crypto_headers})
file(TIMESTAMP "${OPENSSL_CRYPTO_LIBRARY}" crypto_library_stamp UTC)
string(SHA256 crypto_probe_id
    "${OPENSSL_CRYPTO_LIBRARY};${crypto_library_stamp};${crypto_header_inputs};${OPENSSL_VERSION};${CMAKE_OSX_DEPLOYMENT_TARGET};${CMAKE_OSX_ARCHITECTURES}")
cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_LIBRARIES OpenSSL::Crypto)
if(APPLE)
    # Reject a library whose minimum OS exceeds the app's deployment target.
    set(CMAKE_REQUIRED_LINK_OPTIONS "-Wl,-fatal_warnings")
endif()
set(crypto_api_probe "SEVENZIP_OPENSSL_REQUIRED_${crypto_probe_id}")
check_c_source_compiles("#define OPENSSL_SUPPRESS_DEPRECATED
    #include <openssl/sha.h>
    #include <openssl/md5.h>
    #include <openssl/kdf.h>
    int main(void) { SHA_CTX a; SHA256_CTX b; SHA512_CTX c; MD5_CTX d;
        unsigned char bytes[16] = {0};
        SHA1_Init(&a); SHA256_Init(&b); SHA512_Init(&c); MD5_Init(&d);
        a.h0 = 0; b.h[0] = 0; c.h[0] = 0; d.A = d.B = d.C = d.D = 0;
        EVP_CIPHER *cipher = EVP_CIPHER_fetch(0, \"AES-256-ECB\", 0); EVP_CIPHER_free(cipher);
        EVP_KDF *p = EVP_KDF_fetch(0, \"PBKDF2\", 0); EVP_KDF_free(p);
        return SHA1_Update(&a, bytes, 0) + SHA256_Update(&b, bytes, 0)
            + SHA512_Update(&c, bytes, 0) + MD5_Update(&d, bytes, 0); }" ${crypto_api_probe})
cmake_pop_check_state()
if(NOT ${crypto_api_probe})
    message(FATAL_ERROR "A deployment-compatible OpenSSL with the AES/SHA/MD5 state API is required; select it with OPENSSL_ROOT_DIR")
endif()
add_library(sevenzip_crypto_provider STATIC
    src/crypto/opensslkdf.cpp src/crypto/opensslkdf.h
    src/crypto/opensslaes.cpp src/crypto/opensslaes.h
    src/crypto/opensslblock.c src/crypto/opensslblock.h
    src/crypto/opensslhash.c src/crypto/opensslhash.h)
set_target_properties(sevenzip_crypto_provider PROPERTIES AUTOMOC OFF AUTOUIC OFF
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
target_link_libraries(sevenzip_crypto_provider PRIVATE OpenSSL::Crypto)
target_compile_definitions(sevenzip_common PRIVATE SEVENZIP_OPENSSL_HASHES)
target_link_libraries(sevenzip_common PRIVATE sevenzip_crypto_provider)
message(STATUS "Required native crypto: OpenSSL ${OPENSSL_VERSION}")
include(${CMAKE_CURRENT_LIST_DIR}/CryptoRuntime.cmake)
set_source_files_properties(src/C/Aes.c src/C/Md5.c src/C/Sha1.c src/C/Sha256.c src/C/Sha512.c
    PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/C")
set_source_files_properties(src/CPP/7zip/Crypto/Pbkdf2HmacSha1.cpp src/CPP/7zip/Crypto/MyAes.cpp
    PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/Crypto")
if(BUILD_TESTING)
    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
    add_test(NAME crypto_runtime_tooling COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/crypto_runtime_tests.py")
    set_tests_properties(crypto_runtime_tooling PROPERTIES TIMEOUT 60)
    add_executable(crypto_pbkdf2_tests tests/crypto_pbkdf2_tests.cpp
        src/CPP/7zip/Crypto/Pbkdf2HmacSha1.cpp)
    target_include_directories(crypto_pbkdf2_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(crypto_pbkdf2_tests PRIVATE sevenzip_crypto_provider OpenSSL::Crypto)
    set_target_properties(crypto_pbkdf2_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF)
    add_test(NAME crypto_pbkdf2 COMMAND crypto_pbkdf2_tests)
    set_tests_properties(crypto_pbkdf2 PROPERTIES TIMEOUT 15)
endif()
