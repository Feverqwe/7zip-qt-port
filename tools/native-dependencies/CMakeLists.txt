cmake_minimum_required(VERSION 3.28)
project(sevenzip_native_dependencies LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/ModuleToolchain.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/StaticAnalysis.cmake")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/toolchain.json"
    CONTENT "{\"compiler\":\"${CMAKE_CXX_COMPILER_ID}\",\"version\":\"${CMAKE_CXX_COMPILER_VERSION}\",\"modules\":$<IF:$<BOOL:${SEVENZIP_ENABLE_DEPENDENCY_MODULES}>,true,false>}\n")

if(NOT SEVENZIP_RUNTIME_SOURCE_DIR)
    message(FATAL_ERROR "SEVENZIP_RUNTIME_SOURCE_DIR must select the prepared native source tree")
endif()

if(SEVENZIP_ENABLE_DEPENDENCY_MODULES)
    add_library(sevenzip_qemu_facade STATIC)
    target_sources(sevenzip_qemu_facade PUBLIC FILE_SET CXX_MODULES
        BASE_DIRS "${SEVENZIP_RUNTIME_SOURCE_DIR}/dependencies"
        FILES "${SEVENZIP_RUNTIME_SOURCE_DIR}/dependencies/qemu.cppm")
    set_target_properties(sevenzip_qemu_facade PROPERTIES POSITION_INDEPENDENT_CODE ON)
    if(MSVC)
        set_property(TARGET sevenzip_qemu_facade PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded)
        target_compile_options(sevenzip_qemu_facade PRIVATE
            /GR- /UNDEBUG /W4 /permissive- /Zc:__cplusplus)
        if(NOT "$ENV{VSCMD_ARG_TGT_ARCH}" MATCHES "^(arm|arm64)$")
            target_compile_options(sevenzip_qemu_facade PRIVATE /Gr)
        endif()
    else()
        target_compile_options(sevenzip_qemu_facade PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_definitions(sevenzip_qemu_facade PRIVATE
            _REENTRANT _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE
            _GLIBCXX_ASSERTIONS=1 _LIBCPP_ENABLE_HARDENING=1
            _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE)
    endif()
    sevenzip_configure_module_cache(sevenzip_qemu_facade)
    install(TARGETS sevenzip_qemu_facade ARCHIVE DESTINATION lib
        CXX_MODULES_BMI DESTINATION bmi)
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/facade-library.txt"
        CONTENT "$<TARGET_FILE:sevenzip_qemu_facade>\n")
endif()
sevenzip_track_analysis_inputs()
