CMAKE_MINIMUM_REQUIRED(VERSION 3.15)
PROJECT(stdml-collective)

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
SET(LIBRARY_OUTPUT_PATH lib)

FIND_PACKAGE(Threads REQUIRED)
INCLUDE(cmake/common.cmake)
INCLUDE(cmake/fetch_stdtracer.cmake)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

IF(DISABLE_CXX11_ABI)
    ADD_DEFINITIONS(-D_GLIBCXX_USE_CXX11_ABI=0)
ENDIF()

INSTALL(DIRECTORY include DESTINATION .)

OPTION(BUILD_LIB "Build static lib." ON)
OPTION(BUILD_SHARED_LIB "Build shared lib." OFF)

IF(BUILD_LIB)
    FILE(GLOB SRCS ${CMAKE_SOURCE_DIR}/src/stdml/*.cpp)
    ADD_LIBRARY(stdml-collective ${SRCS})
    TARGET_LINK_LIBRARIES(stdml-collective Threads::Threads)
    TARGET_COMPILE_OPTIONS(stdml-collective PRIVATE -fcoroutines)
    INSTALL(TARGETS stdml-collective ARCHIVE DESTINATION lib)
ENDIF()

IF(BUILD_SHARED)
    FILE(GLOB SRCS ${CMAKE_SOURCE_DIR}/src/stdml/*.cpp)
    ADD_LIBRARY(stdml-collective-shared SHARED ${SRCS})
    TARGET_LINK_LIBRARIES(stdml-collective-shared Threads::Threads)
    TARGET_COMPILE_OPTIONS(stdml-collective-shared PRIVATE -fcoroutines)
    INSTALL(TARGETS stdml-collective-shared LIBRARY DESTINATION lib)
    SET_TARGET_PROPERTIES(stdml-collective-shared PROPERTIES OUTPUT_NAME
                                                             stdml-collective)
ENDIF()

IF(HAVE_MPI)
    FIND_PACKAGE(MPI REQUIRED)
ENDIF()

INCLUDE(cmake/examples.cmake)
INCLUDE(cmake/tests.cmake)
INCLUDE(cmake/benchmarks.cmake)
INCLUDE(cmake/pack.cmake)
