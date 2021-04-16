#pragma once
#include <cstddef>

#include <stdml/bits/collective/session.hpp>

namespace stdml::collective
{
size_t run_graph_pair_list_go_rt(session *sess, const workspace &w,
                                 const graph_pair_list &gps, size_t chunk_size);
}
