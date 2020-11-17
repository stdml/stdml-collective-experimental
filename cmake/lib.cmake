OPTION(BUILD_LIB "Build static lib." ON)
OPTION(BUILD_SHARED_LIB "Build shared lib." OFF)

FILE(GLOB SRCS ${CMAKE_SOURCE_DIR}/src/stdml/collective/*.cpp)

IF(BUILD_LIB)
    ADD_LIBRARY(stdml-collective ${SRCS})
    TARGET_LINK_LIBRARIES(stdml-collective Threads::Threads)
    TARGET_COMPILE_OPTIONS(stdml-collective PRIVATE -fcoroutines)
    INSTALL(TARGETS stdml-collective ARCHIVE DESTINATION lib)
    SET_PROPERTY(TARGET stdml-collective PROPERTY CXX_STANDARD 20)
ENDIF()

IF(BUILD_SHARED)
    ADD_LIBRARY(stdml-collective-shared SHARED ${SRCS})
    TARGET_LINK_LIBRARIES(stdml-collective-shared Threads::Threads)
    TARGET_COMPILE_OPTIONS(stdml-collective-shared PRIVATE -fcoroutines)
    INSTALL(TARGETS stdml-collective-shared LIBRARY DESTINATION lib)
    SET_PROPERTY(TARGET stdml-collective-shared PROPERTY CXX_STANDARD 20)
    SET_TARGET_PROPERTIES(stdml-collective-shared PROPERTIES OUTPUT_NAME
                                                             stdml-collective)
ENDIF()
