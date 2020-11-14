#include <stdio.h>

#ifdef USE_MPI
    #include <mpi.h>
#else
    #include <stdml/mpi.h>
#endif

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int rank;
    int size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    printf("rank: %d, size: %d\n", rank, size);

    MPI_Finalize();
    return 0;
}
