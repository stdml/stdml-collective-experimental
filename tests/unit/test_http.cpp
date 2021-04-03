#include <iostream>
#include <string>

#if defined(__APPLE__)
namespace stdml::collective
{
bool http_parse_url(std::string url, std::string &host, uint16_t &port,
                    std::string &path)
{
    return true;
}

std::string http_get(const char *host, uint16_t port, const char *path)
{
    return "";
}
}  // namespace stdml::collective
#else

namespace stdml::collective
{
bool http_parse_url(std::string url, std::string &host, uint16_t &port,
                    std::string &path);

std::string http_get(const char *host, uint16_t port, const char *path);
}  // namespace stdml::collective
#endif

int main()
{
    auto s = stdml::collective::http_get("192.168.1.87", 80, "/");
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
