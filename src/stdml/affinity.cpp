#include <numeric>
#include <ranges>
#include <string>
#include <vector>

#include <sched.h>

#include <stdml/bits/affinity.hpp>
#include <stdml/bits/log.hpp>

std::string show(const std::vector<int> &arr)
{
    std::string s;
    for (auto x : arr) {
        if (!s.empty()) { s += ","; }
        s += std::to_string(x);
    }
    return s;
}

namespace stdml::collective
{
// std::vector<int> select_cpus(const std::vector<int> &all_cpus,
//                              const size_t numa_node_count,
//                              const size_t local_rank, const size_t
//                              local_size)
// {
// }

void bind_to(const std::vector<int> &cpu_ids)
{
    cpu_set_t cpu;
    CPU_ZERO(&cpu);
    for (auto i : cpu_ids) { CPU_SET(i, &cpu); }
    sched_setaffinity(0, sizeof(cpu_set_t), &cpu);
}

// int set_affinity(const std::vector<int> &cpu_order,
//                  const size_t numa_node_count, const size_t local_rank,
//                  const size_t local_size)
// {
//     const auto selected_cpus =
//         select_cpus(cpu_order, numa_node_count, local_rank, local_size);

//     cpu_set_t cpu;
//     CPU_ZERO(&cpu);
//     for (auto i : selected_cpus) { CPU_SET(i, &cpu); }
//     return sched_setaffinity(0, sizeof(cpu_set_t), &cpu);
// }

void set_affinity(int rank, int size)
{
    int nproc = 12;
    std::vector<int> cpus(nproc);
    std::iota(cpus.begin(), cpus.end(), 0);
    // set_affinity(cpus, rank, size);

    std::vector<int> selected_cpus(nproc / size);
    for (auto i : std::views::iota((size_t)0, selected_cpus.size())) {
        selected_cpus[i] = rank * selected_cpus.size() + i;
    }
    log() << "bind to" << show(selected_cpus);
    bind_to(selected_cpus);
}
}  // namespace stdml::collective
