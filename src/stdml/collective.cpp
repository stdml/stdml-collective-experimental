#include <stdml/bits/session.hpp>
#include <stdml/bits/topology.hpp>

namespace stdml::collective
{
void session::run_graphs(const workspace &w,
                         const std::vector<const graph *> &gs)
{
    for (const auto g : gs) {
        //
    }
}

void session::run_graph_pair_list(const workspace &w,
                                  const graph_pair_list &gps)
{
    // TODO: partition workspace evenly and run them on the graph pair list
    const auto &p0 = gps.pairs[0];
    run_graphs(w, {&p0.reduce_graph, &p0.broadcast_graph});
}

void session::all_reduce(const void *input, void *output, size_t count,
                         dtype dt, reduce_op op)
{
    workspace w = {
        .send = input,
        .recv = output,
        .count = count,
        .dt = dt,
        .op = op,
        .name = "",
    };
    run_graph_pair_list(w, all_reduce_topo_);
}
}  // namespace stdml::collective
