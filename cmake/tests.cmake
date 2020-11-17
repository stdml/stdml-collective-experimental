ADD_BINARY(tests/integration/test_all_reduce.cpp)

ADD_EXECUTABLE(test-c++17 tests/build/c++17.cpp)
SET_PROPERTY(TARGET test-c++17 PROPERTY CXX_STANDARD 17)

ENABLE_TESTING()
