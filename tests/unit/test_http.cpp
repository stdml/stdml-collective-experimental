#include <iostream>
#include <string>

#include <stdml/bits/collective/http.hpp>

int main()
{
    auto [_, s] = stdml::collective::http_get("192.168.1.87", 80, "/");
    std::cout << s << std::endl;

    std::string host;
    uint16_t port;
    std::string path;
    stdml::collective::http_parse_url("http://127.0.0.1:80/p", host, port,
                                      path);
    std::cout << host << std::endl;
    std::cout << port << std::endl;
    std::cout << path << std::endl;
    return 0;
}
