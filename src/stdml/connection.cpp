#include <experimental/net>

#include <stdml/bits/connection.hpp>

namespace net = std::experimental::net;

namespace stdml::collective
{
class connection_impl : public connection
{
    using endpoint = net::ip::basic_endpoint<net::ip::tcp>;

    net::io_context ctx_;
    net::ip::tcp::socket socket_;

  public:
    connection_impl(const peer_id id) : socket_(ctx_)
    {
        const auto addr = net::ip::make_address(id.hostname());
        const endpoint ep(addr, id.port);
        net::connect(socket_, std::vector<endpoint>({ep}));
    }
};

connection *connection::dial(const peer_id id)
{
    return new connection_impl(id);
}
}  // namespace stdml::collective
