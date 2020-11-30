#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

namespace stdml::collective::json
{
inline std::string to_json(const std::string &s)
{
    std::stringstream ss;
    ss << '"';
    ss << s;
    ss << '"';
    return ss.str();
}

inline std::string to_json(const int x)
{
    return std::to_string(x);
}

template <typename K, typename V>
std::string to_json(const std::pair<K, V> &p)
{
    std::stringstream ss;
    ss << to_json(p.first);
    ss << ':';
    ss << to_json(p.second);
    return ss.str();
}
}  // namespace stdml::collective::json
