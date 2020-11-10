#include <cstdio>
#include <cstdlib>
#include <experimental/net>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <stdml/collective>

namespace net = std::experimental::net;

static std::vector<std::string> split(const std::string &text, const char sep)
{
    std::vector<std::string> lines;
    std::string line;
    std::istringstream ss(text);
    while (std::getline(ss, line, sep)) {
        if (!line.empty()) { lines.push_back(line); }
    }
    return lines;
}

uint32_t pack(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(b) << 16) |
           (static_cast<uint32_t>(c) << 8) | (static_cast<uint32_t>(d));
}

std::string safe_getenv(const char *name)
{
    const char *ptr = std::getenv(name);
    if (ptr) { return std::string(ptr); }
    return "";
}

namespace stdml::collective
{
class server_impl : public server
{
    const peer_id self_;

    net::io_context ctx_;
    net::ip::tcp::acceptor acceptor_;

    std::unique_ptr<std::thread> thread_;

    void serve(const peer_id &id)
    {
        using endpoint = net::ip::basic_endpoint<net::ip::tcp>;
        const auto addr = net::ip::make_address(id.hostname().c_str());
        const endpoint ep(addr, id.port);

        std::experimental::net::io_context ctx_;
        std::experimental::net::ip::tcp::acceptor acceptor_{ctx_};

        acceptor_.open(ep.protocol());
        acceptor_.bind(ep);
        acceptor_.listen(5);
        for (;;) {
            auto socket = acceptor_.accept();
            std::cout << "after accept" << std::endl;
            const char msg[] = "HTTP/1.1 200 OK\r\n\r\nOK\n";
            net::const_buffer buf(msg, strlen(msg));
            socket.write_some(buf);
            socket.close();
        }
    }

  public:
    server_impl(const peer_id self) : self_(self), acceptor_(ctx_) {}

    ~server_impl()
    {
        std::cout << __func__ << std::endl;
        stop();
    }

    void start() override
    {
        std::cout << "starting server .. " << std::endl;
        thread_.reset(new std::thread([this] {  //
            serve(self_);
        }));
    }

    void stop() override
    {
        if (thread_.get()) {
            std::cout << "joining .. " << std::endl;
            acceptor_.cancel();
            std::cout << "canceled .. " << std::endl;
            // acceptor_.close();
            // std::cout << "closed .. " << std::endl;
            thread_->join();  //
            std::cout << "joined. " << std::endl;
        }
    }
};

std::optional<peer_id> parse_peer_id(const std::string &s)
{
    uint8_t a, b, c, d;
    uint16_t p;
    if (sscanf(s.c_str(), "%hhu.%hhu.%hhu.%hhu:%hu", &a, &b, &c, &d, &p) == 5) {
        return peer_id{
            .ipv4 = pack(a, b, c, d),
            .port = p,
        };
    }
    return {};
}

std::optional<peer_list> parse_peer_list(const std::string &s)
{
    peer_list ps;
    for (const auto &p : split(s, ',')) {
        const auto id = parse_peer_id(p);
        if (id) {
            ps.push_back(id.value());
        } else {
            return {};
        }
    }
    return ps;
}

peer peer::single()
{
    const auto id = parse_peer_id("127.0.0.1:10000").value();
    return peer(id, {id});
}

peer peer::from_env()
{
    const auto self = parse_peer_id(safe_getenv("KUNGFU_SELF_SPEC"));
    if (!self) { return single(); }
    const auto peers = parse_peer_list(safe_getenv("KUNGFU_INIT_PEERS"));
    if (!peers) { return single(); }
    return peer(self.value(), peers.value());
}

void peer::start()
{
    server_.reset(new server_impl(self_));
    server_->start();
}

void peer::stop()
{
    std::cout << "stop peer" << std::endl;
    server_.reset(nullptr);
}
}  // namespace stdml::collective
