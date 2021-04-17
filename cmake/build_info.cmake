EXECUTE_PROCESS(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE)

EXECUTE_PROCESS(
    COMMAND date +%s
    OUTPUT_VARIABLE BUILD_TIME
    OUTPUT_STRIP_TRAILING_WHITESPACE)

EXECUTE_PROCESS(
    COMMAND ${CMAKE_C_COMPILER} --version
    OUTPUT_VARIABLE BUILD_CC_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)

EXECUTE_PROCESS(
    COMMAND ${CMAKE_CXX_COMPILER} --version
    OUTPUT_VARIABLE BUILD_CXX_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)

MESSAGE("BUILD_CC_VERSION: ${BUILD_CC_VERSION}")

MESSAGE("BUILD_CXX_VERSION: ${BUILD_CXX_VERSION}")

SET(GIT_COMMIT "\"${GIT_COMMIT}\"")
SET(STDML_BUILD_CC "\"${CMAKE_C_COMPILER}\"")
SET(STDML_BUILD_CXX "\"${CMAKE_CXX_COMPILER}\"")

FUNCTION(SET_TARGET_BUILD_INFO target)
    TARGET_COMPILE_DEFINITIONS(${target} PRIVATE STDML_COLLECTIVE_GIT_COMMIT=${GIT_COMMIT})
    TARGET_COMPILE_DEFINITIONS(${target} PRIVATE STDML_COLLECTIVE_BUILD_TIME=${BUILD_TIME})

    TARGET_COMPILE_DEFINITIONS(${target}
                               PRIVATE STDML_COLLECTIVE_BUILD_CC=${STDML_BUILD_CC})
    TARGET_COMPILE_DEFINITIONS(${target}
                               PRIVATE STDML_COLLECTIVE_BUILD_CXX=${STDML_BUILD_CXX})
ENDFUNCTION()
