include_guard(GLOBAL)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.43)
        message(FATAL_ERROR "C++23 mode requires MSVC 19.43+ (Visual Studio 2022 17.13+)")
    endif()
    # CMake can map C++23 to the unbounded /std:c++latest even when the
    # compiler has a dedicated C++23 mode. Keep final C++23 mappings if present.
    if(CMAKE_CXX23_STANDARD_COMPILE_OPTION MATCHES "c\\+\\+latest")
        set(CMAKE_CXX23_STANDARD_COMPILE_OPTION "/std:c++23preview")
        set(CMAKE_CXX23_EXTENSION_COMPILE_OPTION "/std:c++23preview")
    endif()
endif()
