#include <iostream>
#include <string>

#include <stdml/bits/collective/ioutil.hpp>
#include <stdml/bits/collective/log.hpp>

namespace net = std::experimental::net;
using stdml::collective::log;

int main()
{
    stdml::collective::enabled_log();

    using tcp_endpoint = net::ip::tcp::endpoint;
    using tcp_socket = net::ip::tcp::socket;
    constexpr bool non_blocking = true;
    using ioutil = stdml::collective::basic_ioutil<tcp_socket, non_blocking>;

    const auto addr = net::ip::make_address("127.0.0.1");
    const tcp_endpoint ep(addr, 80);

    net::io_context ctx;
    tcp_socket socket(ctx);
    socket.connect(ep);
    const std::string req("GET / HTTP/1.1"
                          "\r\n"
                          "Host: 127.0.0.1:80"
                          "\r\n"
                          "User-Agent: curl/7.64.1"
                          "\r\n"
                          "Accept: */*"
                          "\r\n"
                          "\r\n");
    log() << "req.size()" << req.size();

    socket.native_non_blocking(non_blocking);
    {
        // ioutil::write(socket, req.data(), req.size());
        auto t = ioutil::new_write_task(socket, req.data(), req.size());
        t->finish();
    }
    log() << "sent";

    std::string resp;
    resp.resize(400);
    {
        // ioutil::read(socket, resp.data(), resp.size());
        auto t = ioutil::new_read_task(socket, resp.data(), resp.size());
        t->finish();
    }
    log() << resp;
    return 0;
}
