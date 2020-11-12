#include <cstdio>
#include <cstdlib>
#include <experimental/net>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <stdml/bits/connection.hpp>
#include <stdml/bits/peer.hpp>
#include <stdml/bits/rchan.hpp>

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
namespace rchan
{
class connection_impl : public connection
{
    using tcp_endpoint = net::ip::basic_endpoint<net::ip::tcp>;
    using tcp_socket = net::ip::tcp::socket;

    rchan::conn_type type_;
    net::io_context ctx_;
    tcp_socket socket_;
    std::mutex mu_;

    static void wait_connect(tcp_socket &socket, const tcp_endpoint &ep)
    {
        for (int i = 0;; ++i) {
            std::error_code ec;
            socket.connect(ep, ec);
            if (!ec) {
                std::cout << "connected to " << ep << " after " << i
                          << " retries" << std::endl;
                break;
            }
            std::cout << "conn to " << ep << " failed : " << ec << std::endl;
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
        }
    }

    template <typename T>
    void _send(const T &t)
    {
        socket_.write_some(net::buffer(&t, sizeof(T)));
    }

    void _send(const void *data, uint32_t size)
    {
        socket_.write_some(net::buffer(data, size));
    }

  public:
    connection_impl(const peer_id remote, const rchan::conn_type type,
                    const peer_id local)
        : type_(type), socket_(ctx_)
    {
        const auto addr = net::ip::make_address(remote.hostname().c_str());
        const tcp_endpoint ep(addr, remote.port);
        wait_connect(socket_, ep);
        rchan::conn_header h = {
            .type = type,
            .src_port = local.port,
            .src_ipv4 = local.ipv4,
        };
        std::cout << "connected to " << (std::string)remote << std::endl;
        socket_.write_some(net::buffer(&h, sizeof(h)));
        std::cout << "upgraded to " << (std::string)remote << " @ " << type
                  << std::endl;
    }

    ~connection_impl() { socket_.close(); }

    void send(const char *name, const void *data, size_t size,
              uint32_t flags) override
    {
        rchan::message_header mh = {
            .name_len = static_cast<uint32_t>(strlen(name)),
            .name = const_cast<char *>(name),
            .flags = flags,
        };
        rchan::message msg = {
            .len = static_cast<uint32_t>(size),
            .data = const_cast<void *>(data),
        };
        {
            std::lock_guard<std::mutex> _(mu_);
            _send(mh.name_len);
            _send(mh.name, mh.name_len);
            _send(mh.flags);
            _send(msg.len);
            _send(msg.data, msg.len);
        }
    }
};

connection *connection::dial(const peer_id remote, const rchan::conn_type type,
                             const peer_id local)
{
    return new connection_impl(remote, type, local);
}

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
        if (it != pool_.end()) { return it->second.get(); }
        auto conn = connection::dial(target, type_, self_);  // FIXME: unblock
        // pool_.insert(target.hash(), std::unique_ptr<connection>(conn));
        pool_[target.hash()].reset(conn);
        return conn;
    }

  public:
    client_impl(const peer_id self, const conn_type type)
        : self_(self), type_(type)
    {
    }

    void send(const peer_id target, const char *name, const void *data,
              size_t size, uint32_t flags) override
    {
        auto conn = require(target);
        conn->send(name, data, size, flags);
    }
};

client *client_pool::require(conn_type type)
{
    std::cout << "require client of type " << type << std::endl;
    std::lock_guard _(mu_);
    if (auto it = clients_.find(type); it != clients_.end()) {
        std::cout << "using existing client of type " << type << std::endl;
        return it->second.get();
    }
    auto client = new client_impl(self_, type);
    std::cout << "created new client of type " << type << std::endl;
    clients_[type].reset(client);
    return client;
}

class server_impl : public server
{
    using tcp_endpoint = net::ip::basic_endpoint<net::ip::tcp>;
    using tcp_socket = net::ip::tcp::socket;
    using tcp_acceptor = net::ip::tcp::acceptor;

    const peer_id self_;

    net::io_context ctx_;
    tcp_acceptor acceptor_;

    std::unique_ptr<std::thread> thread_;

    static void wait_bind(tcp_acceptor &acceptor, const tcp_endpoint &ep)
    {
        for (int i = 0;; ++i) {
            std::error_code ec;
            acceptor.bind(ep, ec);
            if (!ec) {
                std::cout << "bind success after " << i << " retries"
                          << std::endl;
                break;
            }
            std::cout << "bind error code: " << ec << std::endl;
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
        }
    }

    void upgrade(tcp_socket &socket)
    {
        rchan::conn_header h;
        socket.read_some(net::buffer(&h, sizeof(h)));
        peer_id src = {
            .ipv4 = h.src_ipv4,
            .port = h.src_port,
        };
        std::cout << "got connection from " << (std::string)src << " of type "
                  << h.type << std::endl;
    }

    template <typename T>
    void _recv(tcp_socket &socket, T &t)
    {
        socket.read_some(net::buffer(&t, sizeof(T)));
    }

    void _recv(tcp_socket &socket, void *data, uint32_t size)
    {
        socket.read_some(net::buffer(data, size));
    }

    void recv_one_msg(tcp_socket &socket)
    {
        rchan::message_header mh;
        std::string name;
        std::string body;

        rchan::message msg;
        {
            _recv(socket, mh.name_len);
            name = std::string(mh.name_len, '\0');
            _recv(socket, name.data(), mh.name_len);
            _recv(socket, mh.flags);
        }
        {
            _recv(socket, msg.len);
            body = std::string(msg.len, '\0');
            _recv(socket, body.data(), msg.len);
        }

        std::cout << "name: " << name << ", body: " << body << std::endl;
    }

    void recv_msgs(tcp_socket &socket, int n)
    {
        for (int i = 0; i < n; ++i) { recv_one_msg(socket); }
    }

    void handle(tcp_socket socket)
    {
        upgrade(socket);
        recv_msgs(socket, 1);
        socket.close();
        std::cout << "handle finished" << std::endl;
    }

    void _accept_loop()
    {
        acceptor_.async_accept([&](std::error_code ec, tcp_socket socket) {
            if (!ec) {
                handle(std::move(socket));
            } else {
            }
            _accept_loop();
        });
    }

    void serve(const peer_id &id)
    {
        const auto addr = net::ip::make_address(id.hostname().c_str());
        const tcp_endpoint ep(addr, id.port);

        acceptor_.open(ep.protocol());
        wait_bind(acceptor_, ep);
        acceptor_.listen(5);

        std::cout << "serving " << ep << std::endl;
        _accept_loop();
        std::cout << "entered loop" << std::endl;
        const auto n = ctx_.run();
        std::cout << "serving finished after " << n << std::endl;
    }

  public:
    server_impl(const peer_id self)
        : ctx_(std::thread::hardware_concurrency()),
          self_(self),
          acceptor_(ctx_)
    {
    }

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
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(2s);
            ctx_.stop();
            thread_->join();  //
            std::cout << "joined. " << std::endl;
        }
    }
};
}  // namespace rchan

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

peer::peer(const peer_id self, const peer_list init_peers)
    : self_(self),
      init_peers_(init_peers),
      clients_(new rchan::client_pool(self_))
{
    start();
}

peer::~peer() { stop(); }

peer peer::single()
{
    const auto id = parse_peer_id("127.0.0.1:10000").value();
    return peer(id, {id});
}

peer peer::from_kungfu_env()
{
    const auto self = parse_peer_id(safe_getenv("KUNGFU_SELF_SPEC"));
    if (!self) { return single(); }
    const auto peers = parse_peer_list(safe_getenv("KUNGFU_INIT_PEERS"));
    if (!peers) { return single(); }
    return peer(self.value(), peers.value());
    // p.start();
    // return std::move(p);
}

peer peer::from_env() { return from_kungfu_env(); }

void peer::start()
{
    server_.reset(new rchan::server_impl(self_));
    server_->start();
}

void peer::stop()
{
    std::cout << "stop peer" << std::endl;
    server_.reset(nullptr);
}
}  // namespace stdml::collective
