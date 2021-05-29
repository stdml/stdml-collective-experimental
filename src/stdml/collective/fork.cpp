#include <vector>

#include <stdml/bits/collective/peer.hpp>

#include <unistd.h>

namespace stdml::collective
{
void _run(int rank, int size, const std::function<void(session &)> &f)
{
    auto peer = peer::from_fork(rank, size);
    auto sess = peer.join();
    f(sess);
}

void prun(int n, const std::function<void(session &)> &f)
{
    std::vector<int> pids;
    for (int i = 0; i < n; ++i) {
        int pid = fork();
        if (pid == 0) {
            _run(i, n, f);
            return;
        }
        pids.push_back(pid);
    }
    for (auto pid : pids) {
        int stat_loc;
        waitpid(pid, &stat_loc, 0);
    }
}

peer peer::from_fork(size_t rank, size_t size)
{
    const auto ps = peer_list::gen(size);
    auto config = parse_system_config_from_env();
    auto self = ps.at(rank);
    peer p(config, 0, self, ps, {});
    p.start();
    return p;
}
}  // namespace stdml::collective
