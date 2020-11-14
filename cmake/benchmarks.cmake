ADD_CXX_BINARY(benchmarks/bench_all_reduce.cpp)

IF(HAVE_MPI)
    FIND_PACKAGE(MPI REQUIRED)

    ADD_EXECUTABLE(example-mpi-openmpi examples/example-mpi.c)
    TARGET_INCLUDE_DIRECTORIES(
        example-mpi-openmpi PRIVATE /usr/lib/x86_64-linux-gnu/openmpi/include)
    TARGET_COMPILE_DEFINITIONS(example-mpi-openmpi PRIVATE -DUSE_MPI)
    TARGET_LINK_LIBRARIES(example-mpi-openmpi mpi)
ENDIF()
