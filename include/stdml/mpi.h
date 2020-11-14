#pragma once
#ifdef __cplusplus
extern "C" {
#endif

enum mpi_comm_t {
    MPI_COMM_WORLD,
};

typedef enum mpi_comm_t MPI_Comm;

enum mpi_datatype_t {
    MPI_FLOAT,
};

typedef enum mpi_datatype_t MPI_Datatype;

enum mpi_op_t {
    MPI_SUM,
};

typedef enum mpi_op_t MPI_Op;

extern int MPI_Init(int *argc, char ***argv);
extern int MPI_Finalize();

extern int MPI_Comm_rank(MPI_Comm comm, int *p);
extern int MPI_Comm_size(MPI_Comm comm, int *p);

extern int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                         MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
#ifdef __cplusplus
}
#endif
