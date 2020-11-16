#include <experimental/net>
#include <iostream>

#include <stdml/bits/connection.hpp>
#include <stdml/bits/ioutil.hpp>
#include <stdml/bits/log.hpp>
#include <stdml/bits/mailbox.hpp>
#include <stdml/bits/peer.hpp>
#include <stdml/bits/rchan.hpp>
#include <stdml/bits/stat.hpp>

namespace net = std::experimental::net;

namespace stdml::collective::rchan
{
template <typename Socket>
class basic_ioutil
{
  public:
    static size_t read(Socket &socket, void *ptr, size_t n)
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
    static size_t read(Socket &socket, T &t)
    {
        return read(socket, &t, sizeof(T));
    }

    static size_t write(Socket &socket, const void *ptr, size_t n)
    {
        size_t sent = 0;
        while (n > 0) {
            auto m = socket.write_some(net::buffer(ptr, n));
            if (m == 0) { break; }
            sent += m;
            n -= m;
            ptr = (char *)(ptr) + m;
        }
        return sent;
    }

    template <typename T>
    static size_t write(Socket &socket, const T &t)
    {
        return write(socket, &t, sizeof(T));
    }
};

using ioutil = basic_ioutil<net::ip::tcp::socket>;

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
            log() << "conn to" << ep << "failed :" << ec  //
                  << "(" << ec.message() << ")";
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
        }
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
            STDML_PROFILE_RATE(__func__, msg.len);
            std::lock_guard<std::mutex> _(mu_);
            ioutil::write(socket_, mh.name_len);
            ioutil::write(socket_, mh.name, mh.name_len);
            ioutil::write(socket_, mh.flags);
            ioutil::write(socket_, msg.len);
            ioutil::write(socket_, msg.data, msg.len);
        }
    }
};

connection *connection::dial(const peer_id remote, const rchan::conn_type type,
                             const peer_id local)
{
    return new connection_impl(remote, type, local);
}

class message_reader_impl : public message_reader
{
    using tcp_socket = net::ip::tcp::socket;

    tcp_socket *socket_;
    uint32_t len_;

  public:
    message_reader_impl(tcp_socket *socket) : socket_(socket), len_(0) {}

    std::optional<received_header> read_header() override
    {
        received_header hdr;
        auto n = ioutil::read(*socket_, hdr.name_len);
        if (n == 0) { return {}; }
        hdr.name.resize(hdr.name_len);
        ioutil::read(*socket_, hdr.name.data(), hdr.name_len);
        ioutil::read(*socket_, hdr.flags);
        ioutil::read(*socket_, hdr.len);
        len_ = hdr.len;
        return hdr;
    }

    bool read_body(void *data) override
    {
        STDML_PROFILE_RATE(__func__, len_);
        ioutil::read(*socket_, data, len_);
        return true;
    }
};

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
        // log(PRINT) << "sent" << size;
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

class default_msg_handler_impl : public msg_handler
{
    using tcp_socket = net::ip::tcp::socket;

  public:
    bool operator()(const peer_id &src, void *_socket) override
    {
        tcp_socket &socket = *(reinterpret_cast<tcp_socket *>(_socket));
        message_reader_impl reader(&socket);
        auto mh = reader.read_header();
        if (!mh.has_value()) { return false; }
        buffer b = alloc_buffer(mh->len);
        reader.read_body(b.data.get());
        return true;
    }
};

template <conn_type type>
class msg_handler_impl;

template <>
class msg_handler_impl<conn_collective> : public msg_handler
{
    using tcp_socket = net::ip::tcp::socket;

    mailbox *mailbox_;
    slotbox *slotbox_;

  public:
    msg_handler_impl(mailbox *mb, slotbox *sb) : mailbox_(mb), slotbox_(sb) {}

    bool operator()(const peer_id &src, void *_socket) override
    {
        tcp_socket &socket = *(reinterpret_cast<tcp_socket *>(_socket));
        message_reader_impl reader(&socket);

        auto mh = reader.read_header();
        if (!mh.has_value()) { return false; }

        if (mh->flags & rchan::message_header::wait_recv_buf) {
            slotbox::S *s = slotbox_->require(src, mh->name);
            void *ptr = s->waitQ.get();
            reader.read_body(ptr);
            s->recvQ.put(ptr);
        } else {
            buffer b = alloc_buffer(mh->len);
            reader.read_body(b.data.get());
            mailbox::Q *q = mailbox_->require(src, mh->name);
            q->put(std::move(b));
        }
        return true;
    }
};

class handler_impl : public handler
{
    using tcp_socket = net::ip::tcp::socket;

    mailbox *mailbox_;
    slotbox *slotbox_;

    std::map<conn_type, std::unique_ptr<msg_handler>> msg_handlers_;

  public:
    handler_impl(mailbox *mb, slotbox *sb) : mailbox_(mb), slotbox_(sb)
    {
        msg_handlers_[conn_collective].reset(
            new msg_handler_impl<conn_collective>(mailbox_, slotbox_));
    }

    size_t operator()(const peer_id src, conn_type type, tcp_socket socket)
    {
        default_msg_handler_impl hh;
        msg_handler *h = &hh;
        auto &handler = msg_handlers_.at(type);
        if (handler.get()) { h = handler.get(); }
        int i = 0;
        for (;; ++i) {
            const bool ok = (*h)(src, &socket);
            if (!ok) { break; }
        }
        socket.close();
        return i;
    }
};

handler *handler::New(mailbox *mb, slotbox *sb)
{
    return new handler_impl(mb, sb);
}

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
            log() << "bind to" << ep << "failed :" << ec  //
                  << "(" << ec.message() << ")";
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
                auto n = (*h)(src, type, std::move(socket));
                log() << "handle finished after" << n << "messages";
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
        // tcp_acceptor::reuse_address option;
        // acceptor_.get_option(option);
        // log() << "option:" << option;

        wait_bind(acceptor_, ep);
        acceptor_.listen(5);

        log() << "serving" << ep;
        _accept_loop();
        const auto n = ctx_.run();
        log() << "serving finished after io_context run" << n << "steps";
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
            log() << "joining threads ..";
            acceptor_.cancel();
            log() << "acceptor anceled .";
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
