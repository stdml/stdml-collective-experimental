#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include <stdml/bits/collective/buffer.hpp>
#include <stdml/bits/collective/http.hpp>
#include <stdml/bits/collective/log.hpp>
#include <stdml/bits/collective/net/tcp.hpp>
#include <stdml/bits/collective/rchan.hpp>

#include <arpa/inet.h>

std::string s(const char *p)
{
    return p;
}

namespace stdml::collective
{
std::pair<int, std::string> http_get(const char *host, uint16_t port,
                                     const char *path)
{
    TODO(__func__);
    throw std::runtime_error("TODO");
}
}  // namespace stdml::collective

namespace stdml::collective::rchan
{
class message_reader_impl : public message_reader
{
    using tcp_socket = TcpSocket;
    // using tcp_socket = net::ip::tcp::socket;
    // using ioutil = basic_ioutil<tcp_socket>;

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
        // std::error_code ec;
        int ec = 0;
        // ioutil::read(*socket_, hdr.name_len, ec);
        // log() << "read_header...";
        socket_->recv(&hdr.name_len, sizeof(hdr.name_len), ec);
        if (ec) {
            // log() << "next is none";
            return {};
        }
        STDML_COLLECTIVE_PROFILE_RATE(__func__, hdr.name_len + 8);

        std::string buf;
        buf.resize(hdr.name_len + 8);
        // ioutil::read(*socket_, buf.data(), buf.size());
        socket_->recv(buf.data(), buf.size());

        hdr.flags = *(uint32_t *)(buf.data() + hdr.name_len);
        hdr.len = *(uint32_t *)(buf.data() + hdr.name_len + 4);
        buf.resize(hdr.name_len);
        hdr.name = std::move(buf);

        len_ = hdr.len;
        return hdr;
    }

    bool read_body(void *data) override
    {
        STDML_COLLECTIVE_PROFILE_RATE(__func__, len_);
        // ioutil::read(*socket_, data, len_);
        socket_->recv(data, len_);
        consumed_ = true;
        return true;
    }
};

class connection_impl : public connection
{
    using tcp_socket = TcpSocket;

    rchan::conn_type type_;
    peer_id src_;

    std::mutex mu_;
    std::unique_ptr<TcpSocket> socket_;

    void wait_connect(const peer_id remote)
    {
        InetAddr addr(remote.port, remote.ipv4);

        for (int i = 0;; ++i) {
            int code;
            code = addr.connect(socket_->fd());
            if (code == 0) {
                log() << "connected to" << remote << "after" << i << "retries";
                break;
            }
            perror("connect");
            log() << "conn to" << remote << "with fd" << socket_->fd()
                  << "failed :" << code << errno;
            socket_.reset(new TcpSocket);  // fixme: reuse!
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
        }
    }

    connection_impl(rchan::conn_type type, peer_id src,
                    std::unique_ptr<tcp_socket> socket)
        : type_(type), src_(src), socket_(std::move(socket))
    {
    }

  public:
    connection_impl(const peer_id remote, const rchan::conn_type type,
                    const peer_id local)
        : type_(type), src_(local), socket_(new TcpSocket)
    {
        log() << "new connection_impl with socket fd:" << socket_->fd();
        wait_connect(remote);
        // TODO:
        upgrade_to(*socket_, type, local);
    }

    static void upgrade_to(tcp_socket &socket, rchan::conn_type type,
                           const peer_id self)
    {
        conn_header h = {
            .type = type,
            .src_port = self.port,
            .src_ipv4 = self.ipv4,
        };
        // ioutil::write(socket, h);
        socket.send(&h, sizeof(h));

        connection_ack ack;
        // ioutil::read(socket, ack);
        socket.recv(&ack, sizeof(ack));
        // TODO: check ack
    }

    // /*
    static connection_impl *upgrade_from(std::unique_ptr<tcp_socket> socket)
    {
        conn_header h;
        socket->recv(&h, sizeof(h));
        // ioutil::read(socket, h);

        peer_id src = {
            .ipv4 = h.src_ipv4,
            .port = h.src_port,
        };
        connection_ack ack = {
            .token = 0,  // TODO: token
        };
        // ioutil::write(socket, ack);
        socket->send(&ack, sizeof(ack));
        return new connection_impl(h.type, src, std::move(socket));
    }
    // */

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
        auto p = new message_reader_impl(socket_.get());
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

            const auto s1 = mh.name_len + msg.len + sizeof(mh.name_len) +
                            sizeof(msg.len) + sizeof(mh.flags);
            byte_buffer buf(s1);
            buf.append(mh.name_len);
            buf.append(mh.name, mh.name_len);
            buf.append(mh.flags);
            buf.append(msg.len);
            socket_->send(buf.data(), buf.size());

            socket_->send(msg.data, msg.len);
        }
        // log() << __func__ << "done with name" << name;
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
    const peer_id self_;
    conn_handler *handler_;

    std::unique_ptr<TcpServer> tcp_server_;

    std::unique_ptr<std::thread> thread_;
    std::vector<std::thread> handle_threads_;

    void start_server(const peer_id &id)
    {
        tcp_server_.reset(new TcpServer(id.port));
        log() << "tcp_server_ created";
        tcp_server_->start();
        log() << "tcp_server_ started";
    }

    void serve_loop()
    {
        for (;;) {
            auto conn = tcp_server_->wait_accept();
            log() << "tcp_server_ accepted";
            if (conn.get() == nullptr) {
                break;
            }
            handle_threads_.emplace_back(
                [h = handler_, socket = conn.release()] {
                    log() << "accepted" << socket->fd() << "in handle thread";
                    auto conn = connection_impl::upgrade_from(
                        std::unique_ptr<TcpSocket>(socket));
                    (*h)(std::unique_ptr<connection>(conn));
                    log() << "handle finished" << socket->fd();
                });
        }
    }

  public:
    server_impl(const peer_id self, conn_handler *handler)
        : self_(self), handler_(handler)
    {
    }

    ~server_impl()
    {
        log() << __func__;
        stop();
    }

    void serve() override
    {
        log() << __func__;
        start_server(self_);
        serve_loop();
        log() << __func__ << "finished";
    }

    void start() override
    {
        log() << "starting server ..";
        start_server(self_);
        thread_.reset(new std::thread([this] {  //
            serve_loop();
            log() << "serve thread finished ";
        }));
    }

    void stop() override
    {
        log() << "server_impl" << __func__;
        if (thread_.get()) {
            log() << "stopping tcp_server";
            tcp_server_->stop();
            log() << "called tcp_server to stop";
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
