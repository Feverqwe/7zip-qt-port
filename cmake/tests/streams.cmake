    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
    add_test(NAME linux_frames COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/linux_frames/linux_frames_tests.py
        ${SEVENZIP_CODEC_RUNTIME_DIR} "$<TARGET_FILE:sevenzip_linux_frames>")
    set_tests_properties(linux_frames PROPERTIES TIMEOUT 45)
    qt_add_executable(linux_frames_stream_tests tests/formats/linux_frames/linux_frames_stream_tests.cpp src/formats/initramfs/linuxframestream.cpp)
    target_include_directories(linux_frames_stream_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(linux_frames_stream_tests PRIVATE sevenzip_panel sevenzip_linux_frames_core Qt6::Test)
    if(WIN32)
        target_link_libraries(linux_frames_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(linux_frames_stream_tests PRIVATE
        "SEVENZIP_FRAME_MODULE=\"$<TARGET_FILE:sevenzip_linux_frames>\""
        "SEVENZIP_FRAME_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_FRAME_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_FRAME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/linux_frames/support/linux_frames_fixture.py\"")
    add_dependencies(linux_frames_stream_tests sevenzip_linux_frames_runtime)
    add_test(NAME linux_frames_stream COMMAND linux_frames_stream_tests -silent)
    set_tests_properties(linux_frames_stream PROPERTIES TIMEOUT 30)
    add_test(NAME tar COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/tar/tar_tests.py
        ${SEVENZIP_CODEC_RUNTIME_DIR})
    set_tests_properties(tar PROPERTIES TIMEOUT 45)
    qt_add_executable(tar_stream_tests tests/formats/tar/tar_stream_tests.cpp)
    target_include_directories(tar_stream_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(tar_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(tar_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(tar_stream_tests PRIVATE
        "SEVENZIP_TAR_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_TAR_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_TAR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/tar/support/tar_fixture.py\"")
    add_test(NAME tar_stream COMMAND tar_stream_tests -silent)
    set_tests_properties(tar_stream PROPERTIES TIMEOUT 30)
    add_test(NAME ihex COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/ihex/ihex_tests.py
        ${SEVENZIP_CODEC_RUNTIME_DIR})
    set_tests_properties(ihex PROPERTIES TIMEOUT 45)
    qt_add_executable(ihex_stream_tests tests/formats/ihex/ihex_stream_tests.cpp)
    target_include_directories(ihex_stream_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(ihex_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(ihex_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(ihex_stream_tests PRIVATE
        "SEVENZIP_IHEX_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_IHEX_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_IHEX_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/ihex/support/ihex_fixture.py\"")
    add_test(NAME ihex_stream COMMAND ihex_stream_tests -silent)
    set_tests_properties(ihex_stream PROPERTIES TIMEOUT 30)
    add_test(NAME uefi COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/uefi/uefi_tests.py
        ${SEVENZIP_CODEC_RUNTIME_DIR})
    set_tests_properties(uefi PROPERTIES TIMEOUT 45)
    qt_add_executable(uefi_stream_tests tests/formats/uefi/uefi_stream_tests.cpp)
    target_include_directories(uefi_stream_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(uefi_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(uefi_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(uefi_stream_tests PRIVATE
        "SEVENZIP_UEFI_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_UEFI_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_UEFI_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/uefi/support/uefi_fixture.py\"")
    add_test(NAME uefi_stream COMMAND uefi_stream_tests -silent)
    set_tests_properties(uefi_stream PROPERTIES TIMEOUT 30)
    add_test(NAME pe COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/pe/pe_tests.py
        ${SEVENZIP_CODEC_RUNTIME_DIR})
    set_tests_properties(pe PROPERTIES TIMEOUT 45)
    qt_add_executable(pe_stream_tests tests/formats/pe/pe_stream_tests.cpp)
    target_include_directories(pe_stream_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(pe_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(pe_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(pe_stream_tests PRIVATE
        "SEVENZIP_PE_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_PE_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_PE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/pe/support/pe_fixture.py\"")
    add_test(NAME pe_stream COMMAND pe_stream_tests -silent)
    set_tests_properties(pe_stream PROPERTIES TIMEOUT 30)
    add_test(NAME elf COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/elf/elf_tests.py
        ${SEVENZIP_CODEC_RUNTIME_DIR})
    set_tests_properties(elf PROPERTIES TIMEOUT 45)
    qt_add_executable(elf_stream_tests tests/formats/elf/elf_stream_tests.cpp)
    target_include_directories(elf_stream_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(elf_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(elf_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(elf_stream_tests PRIVATE
        "SEVENZIP_ELF_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_ELF_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_ELF_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/elf/support/elf_fixture.py\"")
    add_test(NAME elf_stream COMMAND elf_stream_tests -silent)
    set_tests_properties(elf_stream PROPERTIES TIMEOUT 30)
    add_test(NAME base64 COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/base64/base64_tests.py
        ${SEVENZIP_CODEC_RUNTIME_DIR})
    set_tests_properties(base64 PROPERTIES TIMEOUT 45)
    qt_add_executable(base64_stream_tests tests/formats/base64/base64_stream_tests.cpp)
    target_include_directories(base64_stream_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(base64_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(base64_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(base64_stream_tests PRIVATE
        "SEVENZIP_BASE64_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_BASE64_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_BASE64_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/base64/support/base64_fixture.py\"")
    add_test(NAME base64_stream COMMAND base64_stream_tests -silent)
    set_tests_properties(base64_stream PROPERTIES TIMEOUT 30)
    add_test(NAME zstd COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/zstd/zstd_tests.py
        ${SEVENZIP_CODEC_RUNTIME_DIR})
    set_tests_properties(zstd PROPERTIES TIMEOUT 45)
    qt_add_executable(zstd_stream_tests tests/formats/zstd/zstd_stream_tests.cpp)
    target_include_directories(zstd_stream_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(zstd_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(zstd_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(zstd_stream_tests PRIVATE
        "SEVENZIP_ZSTD_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_ZSTD_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_ZSTD_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/zstd/support/zstd_fixture.py\"")
    add_test(NAME zstd_stream COMMAND zstd_stream_tests -silent)
    set_tests_properties(zstd_stream PROPERTIES TIMEOUT 30)
    add_test(NAME xz COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/xz/xz_tests.py
        ${SEVENZIP_CODEC_RUNTIME_DIR})
    set_tests_properties(xz PROPERTIES TIMEOUT 45)
    qt_add_executable(xz_stream_tests tests/formats/xz/xz_stream_tests.cpp)
    target_include_directories(xz_stream_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(xz_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(xz_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(xz_stream_tests PRIVATE
        "SEVENZIP_XZ_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_XZ_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_XZ_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/xz/support/xz_fixture.py\"")
    add_test(NAME xz_stream COMMAND xz_stream_tests -silent)
    set_tests_properties(xz_stream PROPERTIES TIMEOUT 30)
    add_test(NAME bzip2 COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/bzip2/bzip2_tests.py
        ${SEVENZIP_CODEC_RUNTIME_DIR})
    set_tests_properties(bzip2 PROPERTIES TIMEOUT 45)
    qt_add_executable(bzip2_stream_tests tests/formats/bzip2/bzip2_stream_tests.cpp)
    target_include_directories(bzip2_stream_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(bzip2_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(bzip2_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(bzip2_stream_tests PRIVATE
        "SEVENZIP_BZIP2_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_BZIP2_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_BZIP2_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/bzip2/support/bzip2_fixture.py\"")
    add_test(NAME bzip2_stream COMMAND bzip2_stream_tests -silent)
    set_tests_properties(bzip2_stream PROPERTIES TIMEOUT 30)
    add_test(NAME lzma COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/lzma/lzma_tests.py
        ${SEVENZIP_CODEC_RUNTIME_DIR})
    set_tests_properties(lzma PROPERTIES TIMEOUT 45)
    qt_add_executable(lzma_stream_tests tests/formats/lzma/lzma_stream_tests.cpp)
    target_include_directories(lzma_stream_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(lzma_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(lzma_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(lzma_stream_tests PRIVATE
        "SEVENZIP_LZMA_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_LZMA_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_LZMA_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/lzma/support/lzma_fixture.py\"")
    add_test(NAME lzma_stream COMMAND lzma_stream_tests -silent)
    set_tests_properties(lzma_stream PROPERTIES TIMEOUT 30)
    add_test(NAME gzip COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/gzip/gzip_tests.py
        ${SEVENZIP_CODEC_RUNTIME_DIR})
    set_tests_properties(gzip PROPERTIES TIMEOUT 45)
    qt_add_executable(gzip_stream_tests tests/formats/gzip/gzip_stream_tests.cpp)
    target_include_directories(gzip_stream_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(gzip_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(gzip_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(gzip_stream_tests PRIVATE
        "SEVENZIP_GZIP_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_GZIP_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_GZIP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/gzip/support/gzip_fixture.py\"")
    add_test(NAME gzip_stream COMMAND gzip_stream_tests -silent)
    set_tests_properties(gzip_stream PROPERTIES TIMEOUT 30)
    add_test(NAME cpio COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/cpio/cpio_tests.py
        ${SEVENZIP_CODEC_RUNTIME_DIR} "$<TARGET_FILE:sevenzip_linux_frames>")
    set_tests_properties(cpio PROPERTIES TIMEOUT 45)
    add_test(NAME initramfs COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/initramfs/initramfs_tests.py
        ${SEVENZIP_CODEC_RUNTIME_DIR} "$<TARGET_FILE:sevenzip_linux_frames>")
    set_tests_properties(initramfs PROPERTIES TIMEOUT 60)
    qt_add_executable(cpio_stream_tests tests/formats/cpio/cpio_stream_tests.cpp)
    target_include_directories(cpio_stream_tests PRIVATE src ${SEVENZIP_7ZIP_SOURCE_DIR}/CPP)
    target_link_libraries(cpio_stream_tests PRIVATE sevenzip_panel Qt6::Test)
    if(WIN32)
        target_link_libraries(cpio_stream_tests PRIVATE oleaut32)
    endif()
    target_compile_definitions(cpio_stream_tests PRIVATE
        "SEVENZIP_CPIO_MODULE=\"$<TARGET_FILE:sevenzip_linux_frames>\""
        "SEVENZIP_INITRAMFS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/initramfs/support/initramfs_fixture.py\""
        "SEVENZIP_CPIO_RUNTIME=\"${SEVENZIP_CODEC_RUNTIME_DIR}\""
        "SEVENZIP_CPIO_PYTHON=\"${Python3_EXECUTABLE}\""
        "SEVENZIP_CPIO_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/formats/cpio/support/cpio_fixture.py\"")
    add_test(NAME cpio_stream COMMAND cpio_stream_tests -silent)
    add_dependencies(cpio_stream_tests sevenzip_linux_frames_runtime)
    set_tests_properties(cpio_stream PROPERTIES TIMEOUT 30)
