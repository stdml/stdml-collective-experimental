OPTION(BUILD_LIB "Build static lib." ON)
OPTION(BUILD_SHARED "Build shared lib." OFF)

FILE(GLOB SRCS ${CMAKE_SOURCE_DIR}/src/stdml/collective/*.cpp
     ${CMAKE_SOURCE_DIR}/src/stdml/collective/*.c)

FUNCTION(SET_LIB_STDML_COLLECTIVE_OPTIONS target)
    SET_PROPERTY(TARGET ${target} PROPERTY CXX_STANDARD 20)
    TARGET_COMPILE_OPTIONS(${target}
                           PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fcoroutines>)
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
ENDIF()

IF(BUILD_SHARED)
    ADD_LIBRARY(stdml-collective-shared SHARED ${SRCS})
    SET_LIB_STDML_COLLECTIVE_OPTIONS(stdml-collective-shared ${SRCS})
    INSTALL(TARGETS stdml-collective-shared LIBRARY DESTINATION lib)
    SET_TARGET_PROPERTIES(stdml-collective-shared PROPERTIES OUTPUT_NAME
                                                             stdml-collective)
ENDIF()
