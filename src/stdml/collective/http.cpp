#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <stdml/bits/collective/http.hpp>

namespace stdml::collective
{
static void trim(std::string &s)
{
    while (!s.empty() && std::isspace(s[0])) {
        s.erase(s.begin());
    }
}

static bool starts_with(const char *s, const char *p)
{
    return strncmp(s, p, strlen(p)) == 0;
}

bool http_parse_url(const std::string &url, std::string &host, uint16_t &port,
                    std::string &path)
{
    // http://host/path
    constexpr const char *prefix = "http://";
    if (!starts_with(url.c_str(), prefix)) {
        return false;
    }
    auto pos = url.find('/', strlen(prefix));
    if (pos == std::string::npos) {
        return false;
    }
    host = url.substr(strlen(prefix), pos - strlen(prefix));
    path = url.substr(pos);
    if (auto pos = host.find(':'); pos != std::string::npos) {
        auto sport = host.substr(pos + 1);
        host = host.substr(0, pos);
        if (sscanf(sport.c_str(), "%hu", &port) != 1) {
            return false;
        }
    } else {
        port = 80;
    }
    return true;
}

std::optional<size_t> http_content_length(const std::vector<std::string> &lines)
{
    for (const auto &line : lines) {
        auto pos = line.find(':');
        if (pos == std::string::npos) {
            continue;
        }
        std::string key(line.begin(), line.begin() + pos);
        std::transform(key.begin(), key.end(), key.begin(),
                       [](char c) { return std::tolower(c); });
        if (key == "content-length") {
            std::string value(line.begin() + pos + 1, line.end());
            trim(value);
            int x;
            if (sscanf(value.c_str(), "%d", &x) == 1) {
                return x;
            }
            break;
        }
    }
    return {};
}

std::string
http_make_request(const char *method, const char *path,
                  const std::vector<std::pair<std::string, std::string>> &kvs)
{
    std::stringstream ss;
    ss << method << " " << path << " HTTP/1.1\r\n";
    for (const auto &[k, v] : kvs) {
        ss << k << ": " << v << "\r\n";
    }
    ss << "\r\n";
    return ss.str();
}

std::string http_make_request(const char *method, const char *host, int port,
                              const char *path)
{
    std::stringstream ss;
    char req_line[1 << 8];
    sprintf(req_line, "%s %s HTTP/1.1", method, path);
    char host_line[1 << 8];
    sprintf(host_line, "Host: %s:%d", host, port);
    ss << req_line << "\r\n"
       << host_line << "\r\n"
       << "\r\n";
    return ss.str();
}
}  // namespace stdml::collective
