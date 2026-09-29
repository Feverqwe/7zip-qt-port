set(TEST_ROOT "${TEST_ROOT}/й resource space")
file(MAKE_DIRECTORY "${TEST_ROOT}")
set(resource "${TEST_ROOT}/test.rc")
set(output "${TEST_ROOT}/test.h")
set(fixture "#include \"TestRes.h\"\nLANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_US\nSTRINGTABLE\nBEGIN\n IDS_ALPHA \"Alpha\"\n // IDS_IGNORED \"Ignored\"\n IDS_BETA \"Beta\"\nEND\n")
file(WRITE "${resource}" "${fixture}")
set(second_resource "${TEST_ROOT}/second.rc")
file(WRITE "${second_resource}" "STRINGTABLE DISCARDABLE\nBEGIN\n IDS_GAMMA \"Gamma\"\nEND\n")
set(inputs "${resource};${second_resource}")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DRESOURCE_FILES=${inputs}" "-DOUTPUT_FILE=${output}"
    -P "${PROJECT_ROOT}/scripts/rc_strings.cmake" RESULT_VARIABLE status)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Valid source STRINGTABLE rejected")
endif()
file(READ "${output}" content)
if(NOT content MATCHES "IDS_ALPHA, L\"Alpha\"" OR
   NOT content MATCHES "IDS_BETA, L\"Beta\"" OR
   NOT content MATCHES "IDS_GAMMA, L\"Gamma\"" OR content MATCHES "IDS_IGNORED")
    message(FATAL_ERROR "Generated captions differ from source")
endif()
file(SHA256 "${output}" initial_hash)
execute_process(COMMAND "${CMAKE_COMMAND}" "-DRESOURCE_FILES=${inputs}" "-DOUTPUT_FILE=${output}"
    -P "${PROJECT_ROOT}/scripts/rc_strings.cmake" RESULT_VARIABLE status)
file(SHA256 "${output}" repeated_hash)
if(NOT status EQUAL 0 OR NOT initial_hash STREQUAL repeated_hash)
    message(FATAL_ERROR "Repeated generation must preserve content")
endif()

foreach(invalid IN ITEMS unsupported unterminated missing_table)
    if(invalid STREQUAL "unsupported")
        file(WRITE "${second_resource}" "STRINGTABLE\nBEGIN\n IDS_ALPHA \"Alpha\"\n UNSUPPORTED_SYNTAX\nEND\n")
    elseif(invalid STREQUAL "unterminated")
        file(WRITE "${second_resource}" "STRINGTABLE\nBEGIN\n IDS_ALPHA \"Alpha\"\n")
    else()
        file(WRITE "${second_resource}" "LANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_US\n")
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DRESOURCE_FILES=${inputs}" "-DOUTPUT_FILE=${output}"
        -P "${PROJECT_ROOT}/scripts/rc_strings.cmake" RESULT_VARIABLE status
        OUTPUT_QUIET ERROR_QUIET)
    file(SHA256 "${output}" after_hash)
    if(status EQUAL 0 OR NOT initial_hash STREQUAL after_hash)
        message(FATAL_ERROR "${invalid} must fail before overwriting valid output")
    endif()
endforeach()

# Mixed Win32 resources opt into extracting tables only; table syntax stays strict.
set(mixed_resource "${TEST_ROOT}/mixed.rc")
set(mixed_fixture "IDR_MENU MENU\nBEGIN\n MENUITEM \"ignored presentation\", IDM_TEST\nEND\nSTRINGTABLE\nBEGIN\n IDS_MIXED \"line\\n{0}\"\nEND\nIDB_TEST BITMAP \"ignored.bmp\"\n")
file(WRITE "${mixed_resource}" "${mixed_fixture}")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DRESOURCE_FILES=${mixed_resource}"
    "-DOUTPUT_FILE=${output}" -P "${PROJECT_ROOT}/scripts/rc_strings.cmake"
    RESULT_VARIABLE status OUTPUT_QUIET ERROR_QUIET)
if(status EQUAL 0)
    message(FATAL_ERROR "Mixed resource must require explicit extraction mode")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" "-DRESOURCE_FILES=${mixed_resource}"
    "-DMIXED_RESOURCE_FILES=${mixed_resource}" "-DOUTPUT_FILE=${output}"
    -P "${PROJECT_ROOT}/scripts/rc_strings.cmake" RESULT_VARIABLE status)
file(READ "${output}" content)
if(NOT status EQUAL 0 OR NOT content MATCHES "IDS_MIXED" OR
   content MATCHES "MENUITEM" OR content MATCHES "IDB_TEST")
    message(FATAL_ERROR "Mixed-resource extraction must emit only source table strings")
endif()
file(SHA256 "${output}" mixed_hash)
execute_process(COMMAND "${CMAKE_COMMAND}" "-DRESOURCE_FILES=${mixed_resource}"
    "-DMIXED_RESOURCE_FILES=${mixed_resource}" "-DOUTPUT_FILE=${output}"
    -P "${PROJECT_ROOT}/scripts/rc_strings.cmake" RESULT_VARIABLE status)
file(SHA256 "${output}" repeated_hash)
if(NOT status EQUAL 0 OR NOT mixed_hash STREQUAL repeated_hash)
    message(FATAL_ERROR "Repeated mixed-resource extraction must preserve content")
endif()
foreach(invalid IN ITEMS unsupported unterminated missing_table undeclared)
    set(mixed_inputs "${mixed_resource}")
    if(invalid STREQUAL "unsupported")
        file(WRITE "${mixed_resource}" "STRINGTABLE\nBEGIN\n INVALID\nEND\n")
    elseif(invalid STREQUAL "unterminated")
        file(WRITE "${mixed_resource}" "STRINGTABLE\nBEGIN\n IDS_MIXED \"Mixed\"\n")
    elseif(invalid STREQUAL "missing_table")
        file(WRITE "${mixed_resource}" "IDR_MENU MENU\nBEGIN\nEND\n")
    else()
        set(mixed_inputs "${TEST_ROOT}/undeclared.rc")
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DRESOURCE_FILES=${mixed_resource}"
        "-DMIXED_RESOURCE_FILES=${mixed_inputs}" "-DOUTPUT_FILE=${output}"
        -P "${PROJECT_ROOT}/scripts/rc_strings.cmake" RESULT_VARIABLE status
        OUTPUT_QUIET ERROR_QUIET)
    file(SHA256 "${output}" after_hash)
    if(status EQUAL 0 OR NOT mixed_hash STREQUAL after_hash)
        message(FATAL_ERROR "Mixed ${invalid} must fail before overwriting valid output")
    endif()
endforeach()
