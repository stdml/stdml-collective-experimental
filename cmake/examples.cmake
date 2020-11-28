FILE(GLOB examples examples/*.cpp)
FOREACH(example ${examples})
    ADD_BINARY(${example})
ENDFOREACH()

IF(HAVE_MPI)
    ADD_EXECUTABLE(example-mpi-openmpi examples/example-mpi.c)
    TARGET_USE_MPI(example-mpi-openmpi)
ENDIF()
