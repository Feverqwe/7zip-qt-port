# libcrypto is confined to the SDK/application side. GPL-2.0-only filesystem
# modules keep their own providers and must not link this target.
set(SEVENZIP_CRYPTO_PROVIDER "AUTO" CACHE STRING "Native crypto provider: AUTO, OPENSSL or UPSTREAM")
set_property(CACHE SEVENZIP_CRYPTO_PROVIDER PROPERTY STRINGS AUTO OPENSSL UPSTREAM)
if(NOT SEVENZIP_CRYPTO_PROVIDER MATCHES "^(AUTO|OPENSSL|UPSTREAM)$")
    message(FATAL_ERROR "Unknown SEVENZIP_CRYPTO_PROVIDER: ${SEVENZIP_CRYPTO_PROVIDER}")
endif()
if(NOT SEVENZIP_CRYPTO_PROVIDER STREQUAL "UPSTREAM")
    # Prefer a self-contained runtime. Explicit dependency/toolchain settings
    # can still select a shared library for a system-managed installation.
    if(NOT DEFINED OPENSSL_USE_STATIC_LIBS)
        set(OPENSSL_USE_STATIC_LIBS TRUE)
    endif()
    if(SEVENZIP_CRYPTO_PROVIDER STREQUAL "OPENSSL")
        find_package(OpenSSL 3 REQUIRED COMPONENTS Crypto)
    else()
        find_package(OpenSSL 3 QUIET COMPONENTS Crypto)
    endif()
endif()
set(sevenzip_openssl_usable FALSE)
if(NOT SEVENZIP_CRYPTO_PROVIDER STREQUAL "UPSTREAM" AND OpenSSL_FOUND)
    set(sevenzip_openssl_usable TRUE)
    file(GLOB sevenzip_openssl_headers CONFIGURE_DEPENDS "${OPENSSL_INCLUDE_DIR}/openssl/*.h")
    set(sevenzip_openssl_header_inputs "")
    foreach(header IN LISTS sevenzip_openssl_headers)
        file(SHA256 "${header}" header_digest)
        string(APPEND sevenzip_openssl_header_inputs "${header}:${header_digest};")
    endforeach()
    string(SHA256 sevenzip_openssl_header_id "${sevenzip_openssl_header_inputs}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${OPENSSL_CRYPTO_LIBRARY}" ${sevenzip_openssl_headers})
    if(APPLE)
        # A Homebrew library can have a newer deployment target than the app.
        # Treat the linker's compatibility warning as a failed dependency probe.
        include(CheckCXXSourceCompiles)
        include(CMakePushCheckState)
        cmake_push_check_state(RESET)
        set(CMAKE_REQUIRED_LIBRARIES OpenSSL::Crypto)
        set(CMAKE_REQUIRED_LINK_OPTIONS "-Wl,-fatal_warnings")
        file(TIMESTAMP "${OPENSSL_CRYPTO_LIBRARY}" sevenzip_openssl_library_stamp UTC)
        string(SHA256 sevenzip_openssl_probe_id
            "${OPENSSL_CRYPTO_LIBRARY};${sevenzip_openssl_library_stamp};${sevenzip_openssl_header_id};${OPENSSL_INCLUDE_DIR};${OPENSSL_VERSION};${CMAKE_OSX_DEPLOYMENT_TARGET};${CMAKE_OSX_ARCHITECTURES}")
        set(sevenzip_openssl_probe "SEVENZIP_OPENSSL_LINK_${sevenzip_openssl_probe_id}")
        check_cxx_source_compiles("#include <openssl/kdf.h>
            int main() { EVP_KDF *kdf = EVP_KDF_fetch(nullptr, \"PBKDF2\", nullptr);
                EVP_KDF_free(kdf); return 0; }" ${sevenzip_openssl_probe})
        set(sevenzip_openssl_usable "${${sevenzip_openssl_probe}}")
        cmake_pop_check_state()
    endif()
    if(NOT sevenzip_openssl_usable)
        if(SEVENZIP_CRYPTO_PROVIDER STREQUAL "OPENSSL")
            message(FATAL_ERROR "libcrypto cannot link for this deployment target; select a compatible OpenSSL build with OPENSSL_ROOT_DIR")
        endif()
        message(STATUS "libcrypto is incompatible with the deployment target; using upstream crypto")
    endif()
endif()
add_library(sevenzip_crypto_provider STATIC src/crypto/opensslkdf.cpp src/crypto/opensslkdf.h
    src/crypto/opensslaes.cpp src/crypto/opensslaes.h)
set_target_properties(sevenzip_crypto_provider PROPERTIES AUTOMOC OFF AUTOUIC OFF
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(sevenzip_openssl_usable)
    target_compile_definitions(sevenzip_crypto_provider PRIVATE SEVENZIP_OPENSSL_CRYPTO)
    target_link_libraries(sevenzip_crypto_provider PRIVATE OpenSSL::Crypto)
    include(CheckCSourceCompiles)
    include(CMakePushCheckState)
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_LIBRARIES OpenSSL::Crypto)
    file(TIMESTAMP "${OPENSSL_CRYPTO_LIBRARY}" sevenzip_openssl_library_stamp UTC)
    string(SHA256 sevenzip_openssl_sha_id
        "${OPENSSL_CRYPTO_LIBRARY};${sevenzip_openssl_library_stamp};${sevenzip_openssl_header_id};${OPENSSL_INCLUDE_DIR};${OPENSSL_VERSION}")
    set(sevenzip_openssl_sha_probe "SEVENZIP_OPENSSL_SHA_${sevenzip_openssl_sha_id}")
    check_c_source_compiles("#define OPENSSL_SUPPRESS_DEPRECATED
        #include <openssl/sha.h>
        int main(void) { SHA_CTX a; SHA256_CTX b; SHA512_CTX c;
            SHA1_Init(&a); SHA256_Init(&b); SHA512_Init(&c);
            a.h0 = 0; b.h[0] = 0; c.h[0] = 0;
            return SHA1_Update(&a, \"\", 0) + SHA256_Update(&b, \"\", 0) + SHA512_Update(&c, \"\", 0); }"
        ${sevenzip_openssl_sha_probe})
    cmake_pop_check_state()
    if(${sevenzip_openssl_sha_probe})
        target_sources(sevenzip_crypto_provider PRIVATE src/crypto/opensslhash.c src/crypto/opensslhash.h)
        target_compile_definitions(sevenzip_crypto_provider PRIVATE SEVENZIP_OPENSSL_HASHES)
        target_compile_definitions(sevenzip_common PRIVATE SEVENZIP_OPENSSL_HASHES)
        target_link_libraries(sevenzip_common PRIVATE sevenzip_crypto_provider)
    else()
        message(STATUS "State-compatible OpenSSL SHA API unavailable; using upstream SHA")
    endif()
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_LIBRARIES OpenSSL::Crypto)
    set(sevenzip_openssl_md5_probe "SEVENZIP_OPENSSL_MD5_${sevenzip_openssl_sha_id}")
    check_c_source_compiles("#define OPENSSL_SUPPRESS_DEPRECATED
        #include <openssl/md5.h>
        int main(void) { MD5_CTX c; MD5_Init(&c); c.A = c.B = c.C = c.D = 0;
            return MD5_Update(&c, \"\", 0); }" ${sevenzip_openssl_md5_probe})
    cmake_pop_check_state()
    if(${sevenzip_openssl_md5_probe})
        target_compile_definitions(sevenzip_crypto_provider PRIVATE SEVENZIP_OPENSSL_MD5)
        target_compile_definitions(sevenzip_common PRIVATE SEVENZIP_OPENSSL_MD5)
        if(NOT ${sevenzip_openssl_sha_probe})
            target_sources(sevenzip_crypto_provider PRIVATE src/crypto/opensslhash.c src/crypto/opensslhash.h)
            target_link_libraries(sevenzip_common PRIVATE sevenzip_crypto_provider)
        endif()
    endif()
    message(STATUS "Native crypto provider: OpenSSL ${OPENSSL_VERSION}")
else()
    message(STATUS "Native crypto provider: upstream")
endif()
include(${CMAKE_CURRENT_LIST_DIR}/CryptoRuntime.cmake)

set_source_files_properties(src/C/Md5.c src/C/Sha1.c src/C/Sha256.c src/C/Sha512.c PROPERTIES
    INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/C")

set_source_files_properties(src/CPP/7zip/Crypto/Pbkdf2HmacSha1.cpp
    src/CPP/7zip/Crypto/MyAes.cpp PROPERTIES
    INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/7zip/CPP/7zip/Crypto")

if(BUILD_TESTING)
    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
    add_test(NAME crypto_runtime_tooling COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/crypto_runtime_tests.py")
    set_tests_properties(crypto_runtime_tooling PROPERTIES TIMEOUT 60)
    add_executable(crypto_pbkdf2_tests tests/crypto_pbkdf2_tests.cpp
        src/CPP/7zip/Crypto/Pbkdf2HmacSha1.cpp
        third_party/7zip/CPP/7zip/Crypto/HmacSha1.cpp
        src/C/Sha1.c third_party/7zip/C/Sha1Opt.c third_party/7zip/C/CpuArch.c)
    target_include_directories(crypto_pbkdf2_tests PRIVATE src third_party/7zip/CPP)
    target_link_libraries(crypto_pbkdf2_tests PRIVATE sevenzip_crypto_provider)
    if(sevenzip_openssl_usable)
        target_link_libraries(crypto_pbkdf2_tests PRIVATE OpenSSL::Crypto)
        target_compile_definitions(crypto_pbkdf2_tests PRIVATE SEVENZIP_TEST_OPENSSL)
    endif()
    set_target_properties(crypto_pbkdf2_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF)
    add_test(NAME crypto_pbkdf2 COMMAND crypto_pbkdf2_tests)
    set_tests_properties(crypto_pbkdf2 PROPERTIES TIMEOUT 15)
endif()
