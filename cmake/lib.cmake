OPTION(BUILD_LIB "Build static lib." ON)
OPTION(BUILD_SHARED_LIB "Build shared lib." OFF)

FILE(GLOB SRCS ${CMAKE_SOURCE_DIR}/src/stdml/collective/*.cpp)

FUNCTION(ADD_LIB_STDML_COLLECTIVE target)
    ADD_LIBRARY(${target} ${ARGN})
    SET_PROPERTY(TARGET ${target} PROPERTY CXX_STANDARD 20)
    TARGET_COMPILE_OPTIONS(${target} PRIVATE -fcoroutines)
    TARGET_LINK_LIBRARIES(${target} Threads::Threads)
    IF(HAVE_GO_RUNTIME)
        TARGET_SOURCES(
            ${target}
            PRIVATE ${CMAKE_SOURCE_DIR}/src/stdml/collective/runtimes/go.cpp)
        TARGET_USE_GO_RUNTIME(${target})
        TARGET_COMPILE_DEFINITIONS(${target}
                                   PRIVATE -DSTDML_COLLECTIVE_HAVE_GO_RUNTIME=1)
    ENDIF()
    INSTALL(TARGETS stdml-collective ARCHIVE DESTINATION lib)
ENDFUNCTION()

IF(BUILD_LIB)
    ADD_LIB_STDML_COLLECTIVE(stdml-collective ${SRCS})
    INSTALL(TARGETS stdml-collective ARCHIVE DESTINATION lib)
ENDIF()

IF(BUILD_SHARED)
    ADD_LIB_STDML_COLLECTIVE(stdml-collective-shared ${SRCS})
    INSTALL(TARGETS stdml-collective-shared LIBRARY DESTINATION lib)
    SET_TARGET_PROPERTIES(stdml-collective-shared PROPERTIES OUTPUT_NAME
                                                             stdml-collective)
ENDIF()
