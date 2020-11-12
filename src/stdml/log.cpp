#include <iostream>
#include <mutex>
#include <thread>

#include <stdml/bits/log.hpp>

namespace stdml::collective
{
std::mutex logger::mu_;

logger::logger() : lk_(mu_), os(std::cout)
{
    os << "[D] " << std::this_thread::get_id();
}

logger::~logger() { os << std::endl; }

logger log() { return logger(); }
}  // namespace stdml::collective
