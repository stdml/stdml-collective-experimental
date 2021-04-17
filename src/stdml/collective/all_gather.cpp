#include <numeric>

#include <stdml/bits/collective/execution.hpp>
#include <stdml/bits/collective/session.hpp>

namespace stdml::collective
{
template <typename T>
std::vector<T> seq(int n)
{
    std::vector<T> s(n);
    std::iota(s.begin(), s.end(), 0);
    return s;
}

void session::all_gather(const void *input, void *output, size_t count,
                         dtype dt, std::string name)
{
    std::vector<workspace> ws;
    ws.reserve(peers_.size());
    for (size_t i = 0; i < peers_.size(); ++i) {
        ws.push_back({
            .send = input,
            .recv = (char *)(output) + count * dtype_size(dt) * i,
            .count = count,
            .dt = dt,
            .op = sum,   // not used
            .name = "",  // not used
        });
    }

    auto par = this->pool();
    fmap(
        par,
        [&](int idx) {
            int rank = idx / 2;
            if (rank == this->rank_) {
                if (idx % 2 == 0) {
                    ws[rank].forward();
                }
                return;
            }
            const auto &id = peers_[rank];
            const auto &w = ws[rank];
            if (idx % 2 == 0) {  // send
                uint32_t flags = rchan::message_header::wait_recv_buf;
                auto client =
                    this->client_pool_->require(rchan::conn_collective);
                client->send(id, name.c_str(), w.send, w.data_size(), flags);
            } else {  // recv
                auto q = this->slotbox_->require(id, name);
                q->get(w.recv);
            }
        },
        seq<int>(peers_.size() * 2));
}
}  // namespace stdml::collective
