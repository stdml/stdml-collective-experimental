#include <chrono>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <stdml/bits/collective/buffer.hpp>
#include <stdml/bits/collective/connection.hpp>
#include <stdml/bits/collective/log.hpp>
#include <stdml/bits/collective/rchan.hpp>
#include <stdml/bits/collective/stat.hpp>

#define REPLICATE(n) for (int _i = 0; _i < static_cast<int>(n); ++_i)

using stdml::collective::log;
using stdml::collective::PRINT;

using C = std::chrono::high_resolution_clock;

namespace stdml::collective
{
void show_build_info();
}

namespace stdml::collective
{
class customized_msg_handler_1 : public rchan::msg_handler
{
    size_t buffer_size_;
    int seq_;

  public:
    customized_msg_handler_1(size_t buffer_size)
        : buffer_size_(buffer_size), seq_(0)
    {
    }

    bool operator()(const rchan::received_header &mh,
                    rchan::message_reader *reader, const peer_id &src) override
    {
        ++seq_;
        if (seq_ % 100 == 0) {
            log() << "#" << seq_ << mh.name << "len:" << mh.len;
        }
        buffer b = alloc_buffer(mh.len);
        reader->read_body(b.data.get());
        return true;
    }
};

class customized_handler_1 : public rchan::conn_handler
{
    size_t buffer_size_;

  public:
    customized_handler_1(size_t buffer_size) : buffer_size_(buffer_size)
    {
    }

    size_t operator()(std::unique_ptr<rchan::connection> conn) override
    {
        customized_msg_handler_1 h_(buffer_size_);
        size_t n = this->handle_to_end(conn.get(), &h_);
        log() << "handled" << n;
        return n;
    }
};
}  // namespace stdml::collective

void main_server(stdml::collective::peer_id &self, size_t buffer_size)
{
    log(PRINT) << __func__ << "";
    stdml::collective::customized_handler_1 h(buffer_size);
    auto srv = stdml::collective::rchan::server::New(self, &h);
    srv->serve();
}

void main_client(stdml::collective::peer_id &target, size_t buffer_size)
{
    log(PRINT) << __func__ << "";
    auto self = stdml::collective::parse_peer_id("0.0.0.0:0").value();
    auto type = stdml::collective::rchan::conn_send;
    auto client = stdml::collective::rchan::client::New(self, type);
    auto conn = client->dial(target, type);
    // log(PRINT) << "dialed conn:" << conn;

    int runs = 100;
    int n = 100;
    REPLICATE(runs)
    {
        LOG_SCOPE_RATE("step", n * buffer_size);
        REPLICATE(n)
        {
            std::string name("hello world");
            std::string buf;
            buf.resize(buffer_size);
            conn->send(name.c_str(), buf.data(), buf.size());
        }
    }
}

// usage: prog s <host:port> <buffer-size>
// usage: prog c <host:port> <buffer-size>
int main(int argc, char *argv[])
{
    stdml::collective::enable_log();
    stdml::collective::show_build_info();
    if (argc < 4) {
        return 1;
    }

    auto id = stdml::collective::parse_peer_id(argv[2]).value();
    size_t buffer_size = std::stoi(argv[3]);

    if (strcmp(argv[1], "s") == 0) {
        main_server(id, buffer_size);
    }
    if (strcmp(argv[1], "c") == 0) {
        main_client(id, buffer_size);
    }
    return 0;
}
