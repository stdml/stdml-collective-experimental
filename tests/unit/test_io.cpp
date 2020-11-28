#include <iostream>
#include <string>

#include <stdml/bits/collective/ioutil.hpp>
#include <stdml/bits/collective/log.hpp>

extern "C" {
extern void set_default_server_socket_opts(int fd);
}

namespace net = std::experimental::net;
using stdml::collective::log;

using tcp_endpoint = net::ip::tcp::endpoint;
using tcp_socket = net::ip::tcp::socket;
using tcp_acceptor = net::ip::tcp::acceptor;
constexpr bool non_blocking = true;
using ioutil = stdml::collective::basic_ioutil<tcp_socket, non_blocking>;

void test_http()
{
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
}

template <typename Socket>
class socket_task : public stdml::collective::task
{
  protected:
    Socket socket_;
    bool finished_;

    using read_task =
        stdml::collective::io_exact_task<Socket, stdml::collective::io_read>;
    using write_task =
        stdml::collective::io_exact_task<Socket, stdml::collective::io_write>;

  public:
    socket_task(Socket socket) : socket_(std::move(socket)), finished_(false)
    {
    }

    bool finished() override
    {
        return finished_;
    }
};

template <typename Socket>
class request_task : public socket_task<Socket>
{
    using P = socket_task<Socket>;
    using P::P;

    std::unique_ptr<stdml::collective::task> task_;
    std::string buf_;

  public:
    void poll() override
    {
        if (this->finished_) {
            return;
        }
        if (task_.get()) {
            task_->poll();
            if (task_->finished()) {
                log() << "finished" << buf_;
                this->socket_.close();
                this->finished_ = true;
            }
        } else {
            buf_.resize(10);
            task_ =
                ioutil::new_read_task(this->socket_, buf_.data(), buf_.size());
        }
    }
};

template <typename Socket>
class response_task : public socket_task<Socket>
{
    using P = socket_task<Socket>;
    using P::P;

    std::unique_ptr<stdml::collective::task> task_;
    std::string buf_;

  public:
    void poll() override
    {
        if (this->finished_) {
            return;
        }
        if (task_.get()) {
            task_->poll();
            if (task_->finished()) {
                this->socket_.close();
                this->finished_ = true;
            }
        } else {
            buf_ = "0123456789";
            task_ =
                ioutil::new_write_task(this->socket_, buf_.data(), buf_.size());
        }
    }
};

template <typename Listener>
class listening_task : public stdml::collective::task
{
    Listener listener_;
    bool finished_;

    size_t remain_;

    std::vector<std::unique_ptr<task>> handler_tasks_;

  public:
    listening_task(Listener listener, size_t stop_after)
        : listener_(std::move(listener)), finished_(false), remain_(stop_after)
    {
    }

    void poll() override
    {
        if (finished_) {
            return;
        }
        if (remain_ > 0) {
            std::error_code ec;
            auto socket = listener_.accept(ec);
            if (!ec) {
                log() << "accepted" << socket.remote_endpoint();
                handler_tasks_.emplace_back(
                    make_task<response_task<tcp_socket>>(std::move(socket)));
                --remain_;
            }
        }
        bool all_done = true;
        for (auto &t : handler_tasks_) {
            t->poll();
            all_done &= t->finished();
        }
        if (all_done) {
            listener_.cancel();
            listener_.release();
            // listener_.close();
            finished_ = true;
        }
    }

    bool finished() override
    {
        return finished_;
    }
};

template <typename Task, typename... Args>
std::unique_ptr<stdml::collective::task> make_task(Args &&... args)
{
    return std::make_unique<Task>(std::forward<Args>(args)...);
}

void test_single_thread()
{
    net::io_context ctx;
    tcp_acceptor acceptor(ctx);
    const auto addr = net::ip::make_address("127.0.0.1");
    const tcp_endpoint ep(addr, 9999);
    acceptor.open(ep.protocol());
    set_default_server_socket_opts(acceptor.native_handle());
    acceptor.bind(ep);
    acceptor.listen(5);
    acceptor.native_non_blocking(true);

    stdml::collective::task_builder ts;
    int n = 5;
    ts << make_task<listening_task<tcp_acceptor>>(std::move(acceptor), n);
    for (int i = 0; i < n; ++i) {
        tcp_socket socket(ctx);
        socket.connect(ep);
        socket.native_non_blocking(true);
        ts << make_task<request_task<tcp_socket>>(std::move(socket));
    }
    auto t = ts.par();
    t->finish();
    delete t;
    ctx.stop();
}

int main()
{
    stdml::collective::enable_log();
    test_http();
    test_single_thread();
    return 0;
}
