ADD_EXECUTABLE(example-1 examples/example-1.cpp)
TARGET_LINK_LIBRARIES(example-1 stdml-collective)
TARGET_COMPILE_OPTIONS(example-1 PRIVATE -fcoroutines)

ADD_EXECUTABLE(example-mpi examples/example-mpi.cpp)
TARGET_LINK_LIBRARIES(example-mpi stdml-collective)
