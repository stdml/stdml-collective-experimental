ADD_BINARY(benchmarks/bench_all_reduce.cpp)
ADD_BINARY(benchmarks/bench_group_all_reduce.cpp)
ADD_BINARY(benchmarks/bench-mpi-all-reduce.c)

IF(HAVE_MPI)
    ADD_EXECUTABLE(bench-mpi-all-reduce-openmpi
                   benchmarks/bench-mpi-all-reduce.c)
    TARGET_USE_MPI(bench-mpi-all-reduce-openmpi)
ENDIF()
