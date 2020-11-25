#include <chrono>

#include <go/sync>
#include <stdml/bits/collective/log.hpp>
#include <stdml/bits/collective/runtimes/go.hpp>

auto _go_rt = go::sync::runtime::New();

namespace stdml::collective
{
void run_graphs_go_rt(session *sess, const workspace *w,
                      const std::vector<const graph *> &gs)
{
    // TODO(__func__);
}

size_t run_graph_pair_list_go_rt(session *sess, const workspace &w,
                                 const graph_pair_list &gps, size_t chunk_size)
{
    using C = std::chrono::high_resolution_clock;
    const auto pw = split_work(w, gps, chunk_size);
    for (auto &[w, gp] : pw) {
        auto t0 = C::now();
        _go_rt->Do([=, w = &w] { run_graphs_go_rt(sess, w, gp); },
                   [=] {
                       auto t1 = C::now();
                       std::chrono::duration<double> d = t1 - t0;
                       printf("done took %.3fus\n", 1e6 * d.count());
                   });
    }
    {
        auto t0 = C::now();
        _go_rt->Wait();
        auto t1 = C::now();
        std::chrono::duration<double> d = t1 - t0;
        printf("wait took %.3fus\n", 1e6 * d.count());
    }
    return pw.size();
}
}  // namespace stdml::collective
