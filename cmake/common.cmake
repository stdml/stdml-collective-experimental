FUNCTION(ADD_BINARY target)
    GET_FILENAME_COMPONENT(name ${target} NAME_WE)
    STRING(REPLACE "_" "-" name ${name})
    ADD_EXECUTABLE(${name} ${target})
    TARGET_LINK_LIBRARIES(${name} stdml-collective)
ENDFUNCTION()

FUNCTION(TARGET_USE_MPI target)
    TARGET_INCLUDE_DIRECTORIES(
        ${target} PRIVATE /usr/lib/x86_64-linux-gnu/openmpi/include)
    TARGET_COMPILE_DEFINITIONS(${target} PRIVATE -DUSE_MPI)
    TARGET_LINK_LIBRARIES(${target} mpi)
ENDFUNCTION()
