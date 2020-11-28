#include <experimental/net>
#include <iostream>
#include <thread>

#include <stdml/bits/collective/connection.hpp>
#include <stdml/bits/collective/ioutil.hpp>
#include <stdml/bits/collective/log.hpp>
#include <stdml/bits/collective/rchan.hpp>
#include <stdml/bits/collective/stat.hpp>

extern "C" {
extern void set_default_server_socket_opts(int fd);
extern void set_default_client_socket_opts(int fd);
}

namespace net = std::experimental::net;

namespace stdml::collective::rchan
{
class message_reader_impl : public message_reader
{
    using tcp_socket = net::ip::tcp::socket;
    using ioutil = basic_ioutil<tcp_socket>;

    tcp_socket *socket_;
    uint32_t len_;
    bool consumed_;

  public:
    message_reader_impl(tcp_socket *socket)
        : socket_(socket), len_(0), consumed_(false)
    {
    }

    // ~message_reader_impl()
    // {
    //     if (!consumed_) {
    //         // error: ‘throw’ will always call ‘terminate’
    //         [-Werror=terminate]
    //         // throw std::runtime_error("message body not consumed");
    //         fprintf(stderr, "message body not consumed\n");
    //         exit(1);
    //     }
    // }

    std::optional<received_header> read_header() override
    {
        received_header hdr;
        std::error_code ec;
        ioutil::read(*socket_, hdr.name_len, ec);
        if (ec) {
            return {};
        }
        hdr.name.resize(hdr.name_len);
        ioutil::read(*socket_, hdr.name.data(), hdr.name_len);
        ioutil::read(*socket_, hdr.flags);
        ioutil::read(*socket_, hdr.len);
        len_ = hdr.len;
        return hdr;
    }

    bool read_body(void *data) override
    {
        STDML_COLLECTIVE_PROFILE_RATE(__func__, len_);
        ioutil::read(*socket_, data, len_);
        consumed_ = true;
        return true;
    }
};

class connection_impl : public connection
{
    using tcp_endpoint = net::ip::tcp::endpoint;
    using tcp_socket = net::ip::tcp::socket;
    using ioutil = basic_ioutil<tcp_socket>;

    rchan::conn_type type_;
    peer_id src_;

    std::mutex mu_;
    net::io_context ctx_;

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

    connection_impl(rchan::conn_type type, peer_id src, tcp_socket socket)
        : type_(type), src_(src), socket_(std::move(socket))
    {
    }

  public:
    connection_impl(const peer_id remote, const rchan::conn_type type,
                    const peer_id local)
        : type_(type), src_(local), socket_(ctx_)
    {
        const auto addr = net::ip::make_address(remote.hostname().c_str());
        const tcp_endpoint ep(addr, remote.port);
        log() << "socket_.native_handle()" << socket_.native_handle();
        wait_connect(socket_, ep);
        set_default_client_socket_opts(socket_.native_handle());
        conn_header h = {
            .type = type,
            .src_port = local.port,
            .src_ipv4 = local.ipv4,
        };
        log() << "connected to" << remote;
        ioutil::write(socket_, h);
        log() << "upgraded to" << remote << "@" << type;
    }

    static connection_impl *upgrade_from(tcp_socket socket)
    {
        conn_header h;
        ioutil::read(socket, h);
        peer_id src = {
            .ipv4 = h.src_ipv4,
            .port = h.src_port,
        };
        return new connection_impl(h.type, src, std::move(socket));
    }

    ~connection_impl()
    {
        socket_.close();
    }

    conn_type type() const override
    {
        return type_;
    }

    peer_id src() const override
    {
        return src_;
    }

    std::unique_ptr<message_reader> next() override
    {
        auto p = new message_reader_impl(&socket_);
        return std::unique_ptr<message_reader>(p);
    }

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
            STDML_COLLECTIVE_PROFILE_RATE(__func__, msg.len);
            ioutil::write(socket_, mh.name_len);
            ioutil::write(socket_, mh.name, mh.name_len);
            ioutil::write(socket_, mh.flags);
            ioutil::write(socket_, msg.len);
            ioutil::write(socket_, msg.data, msg.len);
        }
    }
};

std::unique_ptr<connection> connection::dial(const peer_id remote,
                                             const rchan::conn_type type,
                                             const peer_id local)
{
    return std::unique_ptr<connection>(
        new connection_impl(remote, type, local));
}

class server_impl : public server
{
    using tcp_endpoint = net::ip::tcp::endpoint;
    using tcp_socket = net::ip::tcp::socket;
    using tcp_acceptor = net::ip::tcp::acceptor;
    using ioutil = basic_ioutil<tcp_socket>;

    const peer_id self_;
    conn_handler *handler_;

    net::io_context ctx_;
    tcp_acceptor acceptor_;

    std::unique_ptr<std::thread> thread_;
    std::vector<std::thread> handle_threads_;

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

    void handle(tcp_socket socket)
    {
        handle_threads_.emplace_back(
            [h = handler_, socket = std::move(socket)]() mutable {
                auto conn = connection_impl::upgrade_from(std::move(socket));
                (*h)(std::unique_ptr<connection>(conn));
            });
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
        log() << "native handle:" << acceptor_.native_handle();
        set_default_server_socket_opts(acceptor_.native_handle());
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
    server_impl(const peer_id self, conn_handler *handler)
        : self_(self),
          handler_(handler),
          ctx_(std::thread::hardware_concurrency()),
          acceptor_(ctx_)
    {
    }

    ~server_impl()
    {
        log() << __func__;
        stop();
    }

    void serve() override
    {
        serve(self_);
    }

    void start() override
    {
        log() << "starting server ..";
        thread_.reset(new std::thread([this] {  //
            serve(self_);
        }));
    }

    void stop() override
    {
        log() << "server_impl" << __func__;
        if (thread_.get()) {
            log() << "joining threads ..";
            acceptor_.cancel();
            log() << "acceptor anceled.";
            ctx_.stop();
            thread_->join();  //
            log() << "serving thread joined.";
            for (auto &th : handle_threads_) {
                th.join();
            }
            log() << "all" << handle_threads_.size()
                  << "handling threads joined.";
        }
    }
};

std::unique_ptr<server> server::New(const peer_id self, conn_handler *handler)
{
    return std::unique_ptr<server>(new server_impl(self, handler));
}
}  // namespace stdml::collective::rchan
