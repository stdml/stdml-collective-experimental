#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace stdml::collective
{
bool http_parse_url(const std::string &url, std::string &host, uint16_t &port,
                    std::string &path);

std::pair<int, std::string> http_get(const char *host, uint16_t port,
                                     const char *path);

void http_put(const char *host, uint16_t port, const char *path,
              const void *ptr, size_t len);
}  // namespace stdml::collective
