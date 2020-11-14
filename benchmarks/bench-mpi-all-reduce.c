#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#ifdef USE_MPI
    #include <mpi.h>
#else
    #include <stdml/mpi.h>
#endif

struct buffer_s {
    float *x;
    float *y;
    int count;
};

typedef struct buffer_s buffer_t;

buffer_t *new_buffer(int count)
{
    buffer_t *p = (buffer_t *)malloc(sizeof(buffer_t));
    p->count = count;
    int data_size = sizeof(float) * count;
    p->x = (float *)malloc(data_size);
    p->y = (float *)malloc(data_size);
    memset(p->x, 0, data_size);
    memset(p->y, 0, data_size);
    return p;
}

void del_buffer(buffer_t *p)
{
    free(p->x);
    free(p->y);
    free(p);
}

struct buffer_list_s {
    buffer_t **bs;
    int n;
};

typedef struct buffer_list_s buffer_list_t;

buffer_list_t *new_buffer_list(const int *sizes, int n)
{
    buffer_list_t *p = (buffer_list_t *)malloc(sizeof(buffer_list_t));
    p->n = n;
    p->bs = (buffer_t **)malloc(sizeof(buffer_t *) * n);
    for (int i = 0; i < n; ++i) { p->bs[i] = new_buffer(sizes[i]); }
    return p;
}

void del_buffer_list(buffer_list_t *p)
{
    for (int i = 0; i < p->n; ++i) { del_buffer(p->bs[i]); }
    free(p->bs);
    free(p);
}

int64_t get_time_us()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return 1000000 * (uint64_t)tv.tv_sec + tv.tv_usec;
}

void bench_step(buffer_list_t *bs)
{
    for (int j = 0; j < bs->n; ++j) {
        float *x = bs->bs[j]->x;
        float *y = bs->bs[j]->y;
        int count = bs->bs[j]->count;
        // printf("i=%d, j=%d, count=%d\n", i, j, count);
        MPI_Allreduce(x, y, count, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
    }
}

void bench_all_reduce(const int *sizes, const int n, int times)
{
    int rank;
    int size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int64_t multiplier = 4 * (size - 1);
    int64_t tot = 0;
    for (int i = 0; i < n; ++i) { tot += sizes[i]; }
    int64_t workload = tot * sizeof(float) * multiplier;

    double mean = 0;
    buffer_list_t *bs = new_buffer_list(sizes, n);
    for (int i = 0; i < times; ++i) {
        int64_t t0 = get_time_us();
        printf("step: %d\n", i + 1);
        bench_step(bs);
        int64_t t1 = get_time_us();

        double rate = ((double)workload / (1 << 30)) / ((t1 - t0) / 1.0e6);
        printf("%.3f GiB/s\n", rate);
        mean += rate;
    }
    mean /= times;
    del_buffer_list(bs);

    printf("FINAL RESULT: %s %.3f GiB/s\n", "<name>", mean);
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int n = 100;
    int sizes[n];
    for (int i = 0; i < n; ++i) { sizes[i] = 1024; }
    bench_all_reduce(sizes, n, 10);
    MPI_Finalize();
    return 0;
}
