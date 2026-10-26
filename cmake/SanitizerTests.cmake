# Apply after every test has received its ordinary deadline and environment.
if(NOT BUILD_TESTING OR NOT SEVENZIP_ENABLE_SANITIZERS OR
        NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|RelWithDebInfo)$")
    return()
endif()

if(MSVC)
    get_filename_component(sevenzip_msvc_runtime_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    function(sevenzip_find_test_runtime_files output)
        set(CMAKE_INSTALL_DEBUG_LIBRARIES TRUE)
        set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
        include(InstallRequiredSystemLibraries)
        file(GLOB asan_runtime "${sevenzip_msvc_runtime_dir}/clang_rt.asan_dynamic-*.dll")
        if(NOT asan_runtime)
            message(FATAL_ERROR "The selected MSVC toolchain is missing its ASan runtime DLL")
        endif()
        set(${output} "${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS};${asan_runtime}" PARENT_SCOPE)
    endfunction()
    sevenzip_find_test_runtime_files(sevenzip_msvc_runtime_files)
    string(JOIN "|" sevenzip_test_runtime_libraries ${sevenzip_msvc_runtime_files})
    # The official console restricts DLL search to application/system folders.
    # Keep the instrumented deployment self-contained instead of weakening it.
    add_custom_target(sevenzip_sanitizer_runtime ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${SEVENZIP_CODEC_RUNTIME_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${sevenzip_msvc_runtime_files}
            "${SEVENZIP_CODEC_RUNTIME_DIR}"
        VERBATIM)
    get_property(sevenzip_runtime_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(runtime_target IN LISTS sevenzip_runtime_targets)
        if(runtime_target MATCHES "_runtime$" AND
                NOT runtime_target STREQUAL "sevenzip_sanitizer_runtime")
            add_dependencies(${runtime_target} sevenzip_sanitizer_runtime)
        endif()
    endforeach()
endif()
get_directory_property(sevenzip_instrumented_tests TESTS)
foreach(sevenzip_test IN LISTS sevenzip_instrumented_tests)
    set_property(TEST ${sevenzip_test} APPEND PROPERTY ENVIRONMENT "SEVENZIP_TEST_SANITIZERS=1")
    get_property(sevenzip_test_timeout TEST ${sevenzip_test} PROPERTY TIMEOUT)
    if(sevenzip_test_timeout)
        math(EXPR sevenzip_test_timeout "${sevenzip_test_timeout} * 3")
        set_property(TEST ${sevenzip_test} PROPERTY TIMEOUT ${sevenzip_test_timeout})
    endif()
    # Reserve two scheduler slots for the larger instrumented codec images.
    get_property(sevenzip_test_processors TEST ${sevenzip_test} PROPERTY PROCESSORS)
    if(NOT sevenzip_test_processors OR sevenzip_test_processors LESS 2)
        set_property(TEST ${sevenzip_test} PROPERTY PROCESSORS 2)
    endif()
    if(MSVC)
        set_property(TEST ${sevenzip_test} APPEND PROPERTY ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:${sevenzip_msvc_runtime_dir}")
        set_property(TEST ${sevenzip_test} APPEND PROPERTY ENVIRONMENT
            "SEVENZIP_TEST_RUNTIME_LIBRARIES=${sevenzip_test_runtime_libraries}")
    endif()
endforeach()
