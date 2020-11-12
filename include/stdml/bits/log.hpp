#pragma once

namespace stdml::collective
{
class logger
{
    static std::mutex mu_;
    std::lock_guard<std::mutex> lk_;
    std::ostream &os;

  public:
    logger();

    ~logger();

    template <typename T>
    logger &operator<<(const T &x)
    {
        os << " " << x;
        return *this;
    }
};

logger log();
}  // namespace stdml::collective
