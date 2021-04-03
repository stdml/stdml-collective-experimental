FIND_PROGRAM(CLANG_TIDY_EXE NAMES "clang-tidy")
IF(CLANG_TIDY_EXE)
    SET(CMAKE_CXX_CLANG_TIDY clang-tidy)
ENDIF()

# -fcoroutines is not supported
#
# ranges is not supported
