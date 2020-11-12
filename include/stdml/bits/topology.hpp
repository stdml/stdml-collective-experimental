#pragma once
#include <set>
#include <vector>

namespace stdml::collective
{
class graph
{
    using V = size_t;
    using vertex_list = std::vector<V>;

    const std::vector<bool> self_loop_;
    const std::vector<vertex_list> prevs_;
    const std::vector<vertex_list> nexts_;

  public:
    graph(std::vector<bool> self_loop,  //
          std::vector<vertex_list> prevs, std::vector<vertex_list> nexts)
        : self_loop_(std::move(self_loop)),
          prevs_(std::move(prevs)),
          nexts_(std::move(nexts))
    {
    }

    const vertex_list &nexts(V i) const { return nexts_[i]; }

    const vertex_list &prevs(V i) const { return prevs_[i]; }

    bool self_loop(V i) const { return self_loop_[i]; }
};

class graph_builder
{
    using V = size_t;
    using vertex_set = std::set<V>;

    std::vector<bool> self_loop_;
    std::vector<vertex_set> nexts_;
    std::vector<vertex_set> prevs_;

    bool edeg_exists(V i, V j);

  public:
    graph_builder(V n);

    bool add_edge(V i, V j);

    graph build(bool reverse = false, bool add_self_loops = false) const;
};

struct graph_pair {
    graph reduce_graph;
    graph broadcast_graph;
};

struct graph_pair_list {
    std::vector<graph_pair> pairs;
};

enum strategy {
    star,
    ring,
    binary_tree,
};

graph_pair_list make_graph_pair_list(strategy s, size_t n);
}  // namespace stdml::collective
