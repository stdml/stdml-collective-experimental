#include <stdml/bits/collective/connection.hpp>
#include <stdml/bits/collective/rchan.hpp>

namespace stdml::collective::rchan
{
class client_impl : public client
{
    const peer_id self_;
    const rchan::conn_type type_;

    std::unordered_map<uint64_t, std::unique_ptr<connection>> pool_;
    std::mutex mu_;

    connection *require(const peer_id target)
    {
        std::lock_guard<std::mutex> _(mu_);
        auto it = pool_.find(target.hash());
        if (it != pool_.end()) {
            return it->second.get();
        }
        auto conn = connection::dial(target, type_, self_);  // FIXME: unblock
        auto p = conn.release();
        pool_[target.hash()].reset(p);
        return p;
    }

  public:
    client_impl(const peer_id self, const conn_type type)
        : self_(self), type_(type)
    {
    }

    std::unique_ptr<connection> dial(peer_id target, conn_type type) override
    {
        return connection::dial(target, type, self_);
    }

    void send(const peer_id target, const char *name, const void *data,
              size_t size, uint32_t flags) override
    {
        auto conn = require(target);
        conn->send(name, data, size, flags);
    }
};

std::unique_ptr<client> client::New(peer_id target, conn_type type)
{
    return std::unique_ptr<client>(new client_impl(target, type));
}

client *client_pool::require(conn_type type)
{
    std::lock_guard _(mu_);
    if (auto it = client_pool_.find(type); it != client_pool_.end()) {
        return it->second.get();
    }
    auto client = new client_impl(self_, type);
    client_pool_[type].reset(client);
    return client;
}
}  // namespace stdml::collective::rchan
