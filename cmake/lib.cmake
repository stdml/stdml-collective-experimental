OPTION(BUILD_LIB "Build static lib." ON)
OPTION(BUILD_SHARED "Build shared lib." OFF)
OPTION(ENABLE_COROUTINE "Enable coroutine." OFF)

FILE(
    GLOB
    SRCS #
    ${CMAKE_SOURCE_DIR}/src/stdml/collective/*.cpp
    ${CMAKE_SOURCE_DIR}/src/stdml/collective/net/*.cpp
    ${CMAKE_SOURCE_DIR}/src/stdml/collective/net/*.c)

FUNCTION(ADD_STDML_COLLECTIVE_SOURCES target)
    IF(APPLE)
        SET(STDML_PLATFORM osx)
    ELSE()
        SET(STDML_PLATFORM linux)
    ENDIF()

    TARGET_SOURCE_TREE(
        ${target}
        ${CMAKE_SOURCE_DIR}/src/stdml/collective/platforms/${STDML_PLATFORM}/*)

    OPTION(USE_STD_CXX_NET "" OFF)
    IF(USE_STD_CXX_NET)
        TARGET_SOURCE_TREE(${target}
                           ${CMAKE_SOURCE_DIR}/src/stdml/collective/net/c++20/*)
    ELSE()
        TARGET_SOURCE_TREE(${target}
                           ${CMAKE_SOURCE_DIR}/src/stdml/collective/net/old/*)
    ENDIF()
ENDFUNCTION()

FUNCTION(SET_LIB_STDML_COLLECTIVE_OPTIONS target)
    SET_PROPERTY(TARGET ${target} PROPERTY CXX_STANDARD 20)
    IF(ENABLE_COROUTINE)
        TARGET_COMPILE_OPTIONS(${target}
                               PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fcoroutines>)
    ENDIF()
    TARGET_LINK_LIBRARIES(${target} Threads::Threads)
    IF(HAVE_GO_RUNTIME)
        TARGET_SOURCES(
            ${target}
            PRIVATE ${CMAKE_SOURCE_DIR}/src/stdml/collective/runtimes/go.cpp)
        TARGET_USE_GO_RUNTIME(${target})
        TARGET_COMPILE_DEFINITIONS(${target}
                                   PRIVATE -DSTDML_COLLECTIVE_HAVE_GO_RUNTIME=1)
    ENDIF()
    IF(ENABLE_ELASTIC)
        TARGET_SOURCES(
            ${target}
            PRIVATE ${CMAKE_SOURCE_DIR}/src/stdml/collective/elastic/elastic.cpp
        )
        TARGET_COMPILE_OPTIONS(${target}
                               PRIVATE -DSTDML_COLLECTIVE_ENABLE_ELASTIC=1)
        # TARGET_LINK_LIBRARIES(${target} kungfu-elastic-cgo)
        TARGET_LINK_LIBRARIES(
            ${target}
            ${CMAKE_SOURCE_DIR}/src/stdml/collective/elastic/libkungfu-elastic-cgo.a
        )
    ENDIF()
ENDFUNCTION()

IF(BUILD_LIB)
    ADD_LIBRARY(stdml-collective ${SRCS})
    SET_LIB_STDML_COLLECTIVE_OPTIONS(stdml-collective)
    INSTALL(TARGETS stdml-collective ARCHIVE DESTINATION lib)
    ADD_STDML_COLLECTIVE_SOURCES(stdml-collective)
ENDIF()

IF(BUILD_SHARED)
    ADD_LIBRARY(stdml-collective-shared SHARED ${SRCS})
    SET_LIB_STDML_COLLECTIVE_OPTIONS(stdml-collective-shared ${SRCS})
    ADD_STDML_COLLECTIVE_SOURCES(stdml-collective-shared)
    INSTALL(TARGETS stdml-collective-shared LIBRARY DESTINATION lib)
    SET_TARGET_PROPERTIES(stdml-collective-shared PROPERTIES OUTPUT_NAME
                                                             stdml-collective)
ENDIF()
