CMAKE_MINIMUM_REQUIRED(VERSION 3.15)
PROJECT(stdml-collective)

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
SET(LIBRARY_OUTPUT_PATH lib)

FIND_PACKAGE(Threads REQUIRED)
INCLUDE(cmake/common.cmake)
INCLUDE(cmake/fetch_stdtracer.cmake)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

SET(BUILD_LIB ON)
IF(BUILD_LIB)
    FILE(GLOB SRCS ${CMAKE_SOURCE_DIR}/src/stdml/*.cpp)
    ADD_LIBRARY(stdml-collective ${SRCS})
    TARGET_LINK_LIBRARIES(stdml-collective Threads::Threads)
    TARGET_COMPILE_OPTIONS(stdml-collective PRIVATE -fcoroutines)
ENDIF()

IF(HAVE_MPI)
    FIND_PACKAGE(MPI REQUIRED)
ENDIF()

INCLUDE(cmake/examples.cmake)
INCLUDE(cmake/tests.cmake)
INCLUDE(cmake/benchmarks.cmake)
