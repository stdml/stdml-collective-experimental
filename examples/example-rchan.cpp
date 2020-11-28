#include <csignal>
#include <cstdio>
#include <iostream>
#include <map>
#include <vector>

#include <stdml/bits/collective/connection.hpp>
#include <stdml/bits/collective/log.hpp>
#include <stdml/bits/collective/rchan.hpp>

using stdml::collective::log;
using stdml::collective::PRINT;

stdml::collective::rchan::server *srv;

void _stop_server(int sig)
{
    log(PRINT) << __func__ << "";
    // TODO: stop srv
}

namespace stdml::collective
{
class customized_handler_1 : public rchan::conn_handler
{
  public:
    void operator()(std::unique_ptr<rchan::connection> conn)
    {
        //
    }
};
}  // namespace stdml::collective

void main_server()
{
    log(PRINT) << __func__ << "";
    stdml::collective::peer_id self = {0, 0};
    stdml::collective::customized_handler_1 h;
    srv = stdml::collective::rchan::server::New(self, &h);

    std::signal(SIGINT, _stop_server);
    std::signal(SIGKILL, _stop_server);
    std::signal(SIGTERM, _stop_server);

    srv->serve();
}

void main_client()
{
    log(PRINT) << __func__ << "";
    // stdml::collective::rchan::client client;
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
