#include <experimental/net>
#include <iostream>

#include <stdml/bits/connection.hpp>
#include <stdml/bits/log.hpp>
#include <stdml/bits/mailbox.hpp>
#include <stdml/bits/peer.hpp>
#include <stdml/bits/rchan.hpp>

namespace net = std::experimental::net;

namespace stdml::collective::rchan
{
class ioutil
{
    using tcp_socket = net::ip::tcp::socket;

  public:
    static size_t read(tcp_socket &socket, void *ptr, size_t n)
    {
        size_t got = 0;
        while (n > 0) {
            auto m = socket.read_some(net::buffer(ptr, n));
            if (m == 0) { break; }
            got += m;
            n -= m;
            ptr = (char *)(ptr) + m;
        }
        return got;
    }

    template <typename T>
    static size_t read(tcp_socket &socket, T &t)
    {
        return read(socket, &t, sizeof(T));
    }
};

class connection_impl : public connection
{
    using tcp_endpoint = net::ip::basic_endpoint<net::ip::tcp>;
    using tcp_socket = net::ip::tcp::socket;

    std::mutex mu_;
    net::io_context ctx_;
    rchan::conn_type type_;
    tcp_socket socket_;

    static void wait_connect(tcp_socket &socket, const tcp_endpoint &ep)
    {
        for (int i = 0;; ++i) {
            std::error_code ec;
            socket.connect(ep, ec);
            if (!ec) {
                log() << "connected to" << ep << "after" << i << "retries";
                break;
            }
            log() << "conn to" << ep << "failed :" << ec;
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
        conn_header h = {
            .type = type,
            .src_port = local.port,
            .src_ipv4 = local.ipv4,
        };
        log() << "connected to" << (std::string)remote;
        socket_.write_some(net::buffer(&h, sizeof(h)));
        log() << "upgraded to" << (std::string)remote << "@" << type;
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
    std::lock_guard _(mu_);
    if (auto it = client_pool_.find(type); it != client_pool_.end()) {
        return it->second.get();
    }
    auto client = new client_impl(self_, type);
    client_pool_[type].reset(client);
    return client;
}

class handler_impl : public handler
{
    using tcp_socket = net::ip::tcp::socket;

    mailbox *mailbox_;

  public:
    static bool recv_one_msg(tcp_socket &socket, received_message &msg)
    {
        auto n = ioutil::read(socket, msg.name_len);
        if (n == 0) { return false; }
        msg.name.reset(new char[msg.name_len]);
        ioutil::read(socket, msg.name.get(), msg.name_len);
        ioutil::read(socket, msg.flags);
        ioutil::read(socket, msg.len);
        msg.data.reset(new char[msg.len]);
        ioutil::read(socket, msg.data.get(), msg.len);
        return true;
    }

    static void recv_msgs(tcp_socket &socket, int n)
    {
        for (int i = 0; i < n; ++i) {
            received_message msg;
            recv_one_msg(socket, msg);
        }
    }

  public:
    handler_impl(mailbox *mb) : mailbox_(mb) {}

    void handle_collective(tcp_socket &socket) {}

    void operator()(const peer_id src, rchan::conn_type type, tcp_socket socket)
    {
        for (int i = 0;; ++i) {  //
            received_message msg;
            const bool ok = recv_one_msg(socket, msg);
            if (!ok) {
                log() << "error after received" << i << "messages";
                break;
            }
            if (type == rchan::conn_collective) {
                std::string name(msg.name.get(), msg.name_len);
                // log() << "got msg of type" << type << "from" << src
                //       << "with name length" << name.size();
                mailbox::Q *q = mailbox_->require(src, name);
                // log() << "required queue for put " << src << "@" << name <<
                // ":"
                //       << q;
                buffer b = {
                    .data = std::move(msg.data),
                    .len = msg.len,
                };
                // log() << "putting msg from" << src;
                q->put(std::move(b));
                // log() << "put msg from" << src << "into queue" << q;
            }
            // log() << "received" << i << "messages"  ;
        }
        // recv_msgs(socket, 1);
        socket.close();
    }
};

handler *handler::New(mailbox *mb) { return new handler_impl(mb); }

class server_impl : public server
{
    using tcp_endpoint = net::ip::basic_endpoint<net::ip::tcp>;
    using tcp_socket = net::ip::tcp::socket;
    using tcp_acceptor = net::ip::tcp::acceptor;

    const peer_id self_;
    handler_impl *handler_;

    net::io_context ctx_;
    tcp_acceptor acceptor_;

    std::unique_ptr<std::thread> thread_;
    std::vector<std::unique_ptr<std::thread>> handle_threads_;

    static void wait_bind(tcp_acceptor &acceptor, const tcp_endpoint &ep)
    {
        for (int i = 0;; ++i) {
            std::error_code ec;
            acceptor.bind(ep, ec);
            if (!ec) {
                log() << "bind success after" << i << "retries";
                break;
            }
            log() << "bind error code:" << ec;
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
        }
    }

    static std::pair<peer_id, rchan::conn_type> upgrade(tcp_socket &socket)
    {
        conn_header h;
        socket.read_some(net::buffer(&h, sizeof(h)));
        peer_id src = {
            .ipv4 = h.src_ipv4,
            .port = h.src_port,
        };
        log() << "got connection of type" << h.type << "from" << src;
        return std::make_pair(src, h.type);
    }

    void handle(tcp_socket socket)
    {
        handle_threads_.push_back(std::make_unique<std::thread>(
            [h = handler_, socket = std::move(socket)]() mutable {
                const auto [src, type] = upgrade(socket);
                (*h)(src, type, std::move(socket));
                log() << "handle finished";
            }));
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

        log() << "serving" << ep;
        _accept_loop();
        log() << "entered loop";
        const auto n = ctx_.run();
        log() << "serving finished after" << n;
    }

  public:
    server_impl(const peer_id self, handler_impl *handler)
        : ctx_(std::thread::hardware_concurrency()),
          self_(self),
          handler_(handler),
          acceptor_(ctx_)
    {
    }

    ~server_impl()
    {
        log() << __func__;
        stop();
    }

    void start() override
    {
        log() << "starting server .. ";
        thread_.reset(new std::thread([this] {  //
            serve(self_);
        }));
    }

    void stop() override
    {
        if (thread_.get()) {
            log() << "joining .. ";
            acceptor_.cancel();
            log() << "canceled .. ";
            ctx_.stop();
            thread_->join();  //
            log() << "serving thread joined.";
            for (auto &th : handle_threads_) { th->join(); }
            log() << "all" << handle_threads_.size()
                  << "handling threads joined.";
        }
    }
};

server *server::New(const peer_id self, handler *handler)
{
    return new server_impl(self, dynamic_cast<handler_impl *>(handler));
}
}  // namespace stdml::collective::rchan
