FILE(GLOB examples examples/*.c examples/*.cpp)

FOREACH(example ${examples})
    ADD_BINARY(${example})
ENDFOREACH()

# TARGET_COMPILE_OPTIONS(example-elastic PRIVATE
# -DSTDML_COLLECTIVE_ENABLE_ELASTIC=1)

IF(HAVE_MPI)
    ADD_EXECUTABLE(example-mpi-openmpi examples/example-mpi.c)
    TARGET_USE_MPI(example-mpi-openmpi)
ENDIF()
