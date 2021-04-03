EXECUTE_PROCESS(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE)

ADD_DEFINITIONS(-DSTDML_COLLECTIVE_GIT_COMMIT=${GIT_COMMIT})
