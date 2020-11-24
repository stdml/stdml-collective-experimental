ADD_BINARY(examples/example-1.cpp)
ADD_BINARY(examples/example-task.cpp)
ADD_BINARY(examples/example-mpi.c)

IF(HAVE_MPI)
    ADD_EXECUTABLE(example-mpi-openmpi examples/example-mpi.c)
    TARGET_USE_MPI(example-mpi-openmpi)
ENDIF()
