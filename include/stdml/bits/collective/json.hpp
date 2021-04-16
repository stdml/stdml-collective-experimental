#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <istream>

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

class json_value
{
  public:
};

class json_obj : public json_value
{
    std::map<std::string, std::unique_ptr<json_value>> value;

  public:
    bool parse(std::istream &is)
    {
        return true;
    }
};

class json_int : public json_value
{
    int64_t value;

  public:
};

class json_list : public json_value
{
    std::vector<std::unique_ptr<json_value>> value;

  public:
};
}  // namespace stdml::collective::json
