#include <stdml/bits/collective/execution.hpp>
#include <stdml/bits/collective/session.hpp>

namespace stdml::collective
{
void session::recv(void *output, size_t count, dtype dt, size_t target,
                   std::string name)
{
    const auto &id = peers_[target];
    auto q = this->slotbox_->require(id, name);
    q->get(output);
}

void session::send(const void *input, size_t count, dtype dt, size_t target,
                   std::string name)
{
    const auto &id = peers_[target];
    uint32_t flags = rchan::message_header::wait_recv_buf;
    auto client = this->client_pool_->require(rchan::conn_collective);
    client->send(id, name.c_str(), input, count * dtype_size(dt), flags);
}
}  // namespace stdml::collective
