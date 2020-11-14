#pragma once
#ifdef __cplusplus
extern "C" {
#endif

enum mpi_comm_t {
    MPI_COMM_WORLD,
};

typedef enum mpi_comm_t mpi_comm_t;

extern int MPI_Init(int *argc, char ***argv);
extern int MPI_Finalize();

extern int MPI_Comm_rank(mpi_comm_t comm, int *p);
extern int MPI_Comm_size(mpi_comm_t comm, int *p);

#ifdef __cplusplus
}
#endif
