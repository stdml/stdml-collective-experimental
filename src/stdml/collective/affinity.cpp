#include <cstddef>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include <stdml/bits/collective/affinity.hpp>
#include <stdml/bits/collective/log.hpp>

extern "C" {
void stdml_collective_set_affinity(int n, int *cpus);
}

static std::string show(const std::vector<int> &arr)
{
    std::string s;
    for (auto x : arr) {
        if (!s.empty()) {
            s += ",";
        }
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
    int nproc = std::thread::hardware_concurrency();
    std::vector<int> cpus(nproc);
    std::iota(cpus.begin(), cpus.end(), 0);

    // set_affinity(cpus, rank, size);
    std::vector<int> selected_cpus;
    int half = cpus.size() / 2;
    if (rank < size / 2) {
        selected_cpus.insert(selected_cpus.begin(), cpus.begin(),
                             cpus.begin() + half);
    } else {
        selected_cpus.insert(selected_cpus.begin(), cpus.begin() + half,
                             cpus.end());
    }
    log() << "bind to" << show(selected_cpus);
    stdml_collective_set_affinity(selected_cpus.size(), selected_cpus.data());
}
}  // namespace stdml::collective
