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

using stdml::collective::log;
using stdml::collective::PRINT;

void _stop_server(int sig)
{
    log(PRINT) << __func__ << "";
    // TODO: stop _srv
}

namespace stdml::collective
{
class customized_msg_handler_1 : public rchan::msg_handler
{
  public:
    bool operator()(const rchan::received_header &mh,
                    rchan::message_reader *reader, const peer_id &src) override
    {
        buffer b = alloc_buffer(mh.len);
        reader->read_body(b.data.get());
        log() << std::string((char *)b.data.get(), mh.len);
        return true;
    }
};

class customized_handler_1 : public rchan::conn_handler
{
  public:
    size_t operator()(std::unique_ptr<rchan::connection> conn) override
    {
        customized_msg_handler_1 h;
        return this->handle_to_end(conn.get(), &h);
    }
};
}  // namespace stdml::collective

void main_server()
{
    log(PRINT) << __func__ << "";
    auto self = stdml::collective::parse_peer_id("127.0.0.1:9999").value();
    stdml::collective::customized_handler_1 h;
    auto srv = stdml::collective::rchan::server::New(self, &h);

    srv->serve();
}

void main_client()
{
    log(PRINT) << __func__ << "";
    auto self = stdml::collective::parse_peer_id("127.0.0.1:8888").value();
    auto target = stdml::collective::parse_peer_id("127.0.0.1:9999").value();
    auto type = stdml::collective::rchan::conn_send;
    auto client = stdml::collective::rchan::client::New(self, type);
    auto conn = client->dial(target, type);
    // log(PRINT) << "dialed conn:" << conn;

    for (int i = 0; i < 10; ++i) {
        std::string buf("hello world!" + std::string(i + 1, '!'));
        conn->send("", buf.data(), buf.size());
    }
}

int main(int argc, char *argv[])
{
    stdml::collective::enable_log();
    if (argc < 2) {
        return 1;
    }
    if (strcmp(argv[1], "s") == 0) {
        main_server();
    } else {
        main_client();
    }
    return 0;
}
