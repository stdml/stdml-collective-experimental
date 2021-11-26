#define _GNU_SOURCE /* See feature_test_macros(7) */

#include <sched.h>

#include <stdio.h>

void stdml_collective_set_affinity(int n, int *cpus)
{
    cpu_set_t cpu;
    CPU_ZERO(&cpu);
    for (int i = 0; i < n; ++i) {
        CPU_SET(i, &cpu);
    }
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu) != 0) {
        perror("sched_setaffinity failed");
    }
}
