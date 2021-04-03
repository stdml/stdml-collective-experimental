#include <algorithm>
#include <cstddef>
#include <ostream>
#include <set>
#include <utility>
#include <vector>

#include <stdml/bits/collective/topology.hpp>

namespace stdml::collective
{
std::ostream &operator<<(std::ostream &os, const graph &g)
{
    const size_t n = g.size();
    os << n << std::endl;
    for (size_t i = 0; i < n; ++i) {
        os << i << "[" << g.self_loop(i) << "]";
        for (auto j : g.nexts(i)) {
            os << " ->" << j;
        }
        for (auto j : g.prevs(i)) {
            os << " <-" << j;
        }
        os << std::endl;
    }
    return os;
}

graph_builder::graph_builder(V n) : self_loop_(n), nexts_(n), prevs_(n)
{
    std::fill(self_loop_.begin(), self_loop_.end(), false);
}

bool graph_builder::edeg_exists(V i, V j)
{
    if (i == j) {
        return self_loop_[i];
    }
    return nexts_[i].count(j) > 0;
}

bool graph_builder::add_edge(V i, V j)
{
    if (edeg_exists(i, j)) {
        return false;
    }
    if (i == j) {
        self_loop_[i] = true;
    } else {
        nexts_[i].insert(j);
        prevs_[j].insert(i);
    }
    return true;
}

template <typename T>
struct sort_set {
    std::vector<T> operator()(const std::set<T> &s) const
    {
        std::vector<T> v(s.size());
        std::copy(s.begin(), s.end(), v.begin());
        return v;
    }
};

graph graph_builder::build(bool reverse, bool add_self_loops) const
{
    const size_t n = self_loop_.size();
    std::vector<std::vector<V>> nexts(n);
    std::vector<std::vector<V>> prevs(n);
    std::transform(nexts_.begin(), nexts_.end(), nexts.begin(), sort_set<V>());
    std::transform(prevs_.begin(), prevs_.end(), prevs.begin(), sort_set<V>());

    if (reverse) {
        std::swap(nexts, prevs);
    }

    if (add_self_loops) {
        std::vector<bool> self_loop(n);
        std::fill(self_loop.begin(), self_loop.end(), true);
        return graph(std::move(self_loop), std::move(nexts), std::move(prevs));
    } else {
        return graph(self_loop_, std::move(nexts), std::move(prevs));
    }
}

graph_builder start_broadcast_graph_builder(size_t n, size_t root)
{
    graph_builder g(n);
    for (size_t i = 0; i < n; ++i) {
        if (i != root) {
            g.add_edge(root, i);
        }
    }
    return g;
}

graph_pair make_circular_graph_pair(size_t n, size_t r)
{
    graph_builder rg(n);
    graph_builder bg(n);
    for (size_t i = 1; i < n; ++i) {
        rg.add_edge((r + i) % n, (r + i + 1) % n);
        bg.add_edge((r + i - 1) % n, (r + i) % n);
    }
    return {rg.build(false, true), bg.build()};
}

graph_pair_list make_graph_pair_list_star(size_t n, size_t root)
{
    graph_builder g = start_broadcast_graph_builder(n, root);
    graph_pair p = {
        .reduce_graph = g.build(true, true),
        .broadcast_graph = g.build(),
    };
    return {.pairs = {p}};
}

graph_pair_list make_graph_pair_list_ring(size_t n)
{
    std::vector<graph_pair> pairs;
    for (size_t i = 0; i < n; ++i) {
        pairs.emplace_back(make_circular_graph_pair(n, i));
    }
    return {pairs};
}

graph_pair_list make_graph_pair_list(strategy s, size_t n)
{
    switch (s) {
    case star:
        return make_graph_pair_list_star(n, 0);
    case ring:
        return make_graph_pair_list_ring(n);
    //
    default:
        return make_graph_pair_list_star(n, 0);
    }
}
}  // namespace stdml::collective
