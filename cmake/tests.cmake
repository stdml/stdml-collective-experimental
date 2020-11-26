ADD_BINARY(tests/unit/test_io.cpp)

ADD_BINARY(tests/integration/test_all_reduce.cpp)
ADD_BINARY(tests/integration/test_shutdown.cpp)

ADD_EXECUTABLE(test-c++17 tests/build/c++17.cpp)
SET_PROPERTY(TARGET test-c++17 PROPERTY CXX_STANDARD 17)

OPTION(USE_CATCH2 "Use Catch2." ON)

IF(USE_CATCH2)
    INCLUDE(cmake/fetch_catch2.cmake)

    FUNCTION(ADD_CATCH2_TEST src)
        GET_FILENAME_COMPONENT(target ${src} NAME_WE)
        STRING(REPLACE "_" "-" target ${target})

        ADD_BINARY(${src})
        TARGET_INCLUDE_DIRECTORIES(${target}
                                   PRIVATE ${catch2_SOURCE_DIR}/single_include)
        TARGET_LINK_LIBRARIES(${target} Catch2::Catch2)
        ADD_TEST(NAME ${target} COMMAND ${target})
    ENDFUNCTION()

    ADD_CATCH2_TEST(tests/catch2/test_1.cpp)
ENDIF()

ENABLE_TESTING()
