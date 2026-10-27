# CMake may retain implicit include paths from an older SDK in a warm tree.
# A dependency such as Iconv can then add the current SDK's C headers ahead of
# libc++ wrappers. Both path spellings below are already implicit in -isysroot.
if(APPLE AND IS_DIRECTORY "${CMAKE_OSX_SYSROOT}/usr/include")
    set(sevenzip_sdk_include "${CMAKE_OSX_SYSROOT}/usr/include")
    file(REAL_PATH "${sevenzip_sdk_include}" sevenzip_sdk_include_real)
    foreach(sevenzip_language C CXX OBJCXX)
        list(APPEND CMAKE_${sevenzip_language}_IMPLICIT_INCLUDE_DIRECTORIES
            "${sevenzip_sdk_include}" "${sevenzip_sdk_include_real}")
        list(REMOVE_DUPLICATES CMAKE_${sevenzip_language}_IMPLICIT_INCLUDE_DIRECTORIES)
    endforeach()
endif()
